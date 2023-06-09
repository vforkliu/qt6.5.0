// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill_assistant/browser/actions/tell_action.h"

#include <utility>

#include "base/callback.h"
#include "components/autofill_assistant/browser/actions/action_delegate.h"

namespace autofill_assistant {

TellAction::TellAction(ActionDelegate* delegate, const ActionProto& proto)
    : Action(delegate, proto) {
  DCHECK(proto_.has_tell());
}

TellAction::~TellAction() {}

void TellAction::InternalProcessAction(ProcessActionCallback callback) {
  if (proto_.tell().has_message()) {
    delegate_->SetStatusMessage(proto_.tell().message());
  }

  if (proto_.tell().needs_ui())
    delegate_->RequireUI();

  if (proto_.tell().has_text_to_speech()) {
    if (proto_.tell().text_to_speech().has_tts_message()) {
      delegate_->SetTtsMessage(proto_.tell().text_to_speech().tts_message());
    }

    if (proto_.tell().text_to_speech().play_now() &&
        delegate_->GetTtsButtonState() != TtsButtonState::DISABLED) {
      delegate_->MaybePlayTtsMessage();
    }
  }

  UpdateProcessedAction(ACTION_APPLIED);
  std::move(callback).Run(std::move(processed_action_proto_));
}

}  // namespace autofill_assistant
