// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill_assistant/browser/web/element_position_getter.h"

#include "components/autofill_assistant/browser/devtools/devtools_client.h"
#include "components/autofill_assistant/browser/service.pb.h"
#include "components/autofill_assistant/browser/web/web_controller_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace {

const char* const kScrollIntoViewIfNeededScript =
    R"(function() {
      this.scrollIntoViewIfNeeded();
    })";

}  // namespace

namespace autofill_assistant {

ElementPositionGetter::ElementPositionGetter(
    DevtoolsClient* devtools_client,
    int max_rounds,
    base::TimeDelta check_interval,
    const std::string& optional_node_frame_id)
    : check_interval_(check_interval),
      max_rounds_(max_rounds),
      devtools_client_(devtools_client),
      node_frame_id_(optional_node_frame_id),
      weak_ptr_factory_(this) {}

ElementPositionGetter::~ElementPositionGetter() = default;

void ElementPositionGetter::Start(content::RenderFrameHost* frame_host,
                                  std::string element_object_id,
                                  Callback callback) {
  callback_ = std::move(callback);
  if (!frame_host) {
    OnError(ClientStatus(FRAME_HOST_NOT_FOUND));
    return;
  }

  object_id_ = element_object_id;
  remaining_rounds_ = max_rounds_;
  // TODO(crbug/806868): Consider using autofill_assistant::RetryTimer

  // Wait for a roundtrips through the renderer and compositor pipeline,
  // otherwise touch event may be dropped because of missing handler.
  // Note that mouse left button will always be send to the renderer, but it
  // is slightly better to wait for the changes, like scroll, to be visualized
  // in compositor as real interaction.
  frame_host->InsertVisualStateCallback(
      base::BindOnce(&ElementPositionGetter::OnVisualStateUpdatedCallback,
                     weak_ptr_factory_.GetWeakPtr()));
  GetAndWaitBoxModelStable();
}

void ElementPositionGetter::OnVisualStateUpdatedCallback(bool success) {
  if (success) {
    visual_state_updated_ = true;
    return;
  }

  OnError(ClientStatus(ELEMENT_UNSTABLE));
}

void ElementPositionGetter::GetAndWaitBoxModelStable() {
  devtools_client_->GetDOM()->GetBoxModel(
      dom::GetBoxModelParams::Builder().SetObjectId(object_id_).Build(),
      node_frame_id_,
      base::BindOnce(&ElementPositionGetter::OnGetBoxModelForStableCheck,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ElementPositionGetter::OnGetBoxModelForStableCheck(
    const DevtoolsClient::ReplyStatus& reply_status,
    std::unique_ptr<dom::GetBoxModelResult> result) {
  if (!result || !result->GetModel() || !result->GetModel()->GetContent() ||
      result->GetModel()->GetWidth() == 0 ||
      result->GetModel()->GetHeight() == 0) {
    VLOG(2) << __func__ << " No box model.";
    RunNextRound();
    return;
  }

  // Return the center of the element.
  const std::vector<double>* content_box = result->GetModel()->GetContent();
  DCHECK_EQ(content_box->size(), 8u);
  int new_point_x = round(((*content_box)[0] + (*content_box)[2]) * 0.5);
  int new_point_y = round(((*content_box)[3] + (*content_box)[5]) * 0.5);

  DCHECK(max_rounds_ >= remaining_rounds_);

  if (has_point_ || max_rounds_ <= 1) {
    // Less than 3 rounds returns immediately, we don't expect stability
    // information to be useful with too few rounds.
    if (max_rounds_ <= 2) {
      OnResult(new_point_x, new_point_y);
      return;
    }

    // If there are enough rounds, wait for at least three rounds (~600ms =
    // 3*check_interval_) for visual state update callback since it might take
    // longer time to return or never return if no updates.
    if (new_point_x == point_x_ && new_point_y == point_y_ &&
        (visual_state_updated_ || remaining_rounds_ + 2 < max_rounds_)) {
      // Note that there is still a chance that the element's position has been
      // changed after the last call of GetBoxModel, however, it might be safe
      // to assume the element's position will not be changed before issuing
      // click or tap event after stable for check_interval_. In addition,
      // checking again after issuing click or tap event doesn't help since the
      // change may be expected.
      OnResult(new_point_x, new_point_y);
      return;
    }
  }

  bool is_first_round = !has_point_;
  has_point_ = true;
  point_x_ = new_point_x;
  point_y_ = new_point_y;

  // Scroll the element into view again if it was moved out of view, starting
  // from the second round.
  if (!is_first_round) {
    devtools_client_->GetRuntime()->CallFunctionOn(
        runtime::CallFunctionOnParams::Builder()
            .SetObjectId(object_id_)
            .SetFunctionDeclaration(std::string(kScrollIntoViewIfNeededScript))
            .SetReturnByValue(true)
            .Build(),
        node_frame_id_,
        base::BindOnce(&ElementPositionGetter::OnScrollIntoView,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  RunNextRound();
}

void ElementPositionGetter::OnScrollIntoView(
    const DevtoolsClient::ReplyStatus& reply_status,
    std::unique_ptr<runtime::CallFunctionOnResult> result) {
  ClientStatus status =
      CheckJavaScriptResult(reply_status, result.get(), __FILE__, __LINE__);
  if (!status.ok()) {
    VLOG(1) << __func__ << " Failed to scroll the element: " << status;
    OnError(status);
    return;
  }

  RunNextRound();
}

void ElementPositionGetter::RunNextRound() {
  if (remaining_rounds_ <= 0) {
    OnError(ClientStatus(has_point_ ? ELEMENT_UNSTABLE
                                    : ELEMENT_POSITION_NOT_FOUND));
    return;
  }

  --remaining_rounds_;
  content::GetUIThreadTaskRunner({})->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ElementPositionGetter::GetAndWaitBoxModelStable,
                     weak_ptr_factory_.GetWeakPtr()),
      check_interval_);
}

void ElementPositionGetter::OnResult(int x, int y) {
  if (callback_) {
    point_x_ = x;
    point_y_ = y;
    std::move(callback_).Run(OkClientStatus());
  }
}

void ElementPositionGetter::OnError(const ClientStatus& status) {
  if (callback_) {
    std::move(callback_).Run(status);
  }
}

}  // namespace autofill_assistant
