// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_ACCESSIBILITY_READ_ANYTHING_APP_CONTROLLER_H_
#define CHROME_RENDERER_ACCESSIBILITY_READ_ANYTHING_APP_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "chrome/common/accessibility/read_anything.mojom.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_node_id_forward.h"
#include "ui/accessibility/ax_tree_update_forward.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace ui {
class AXNode;
class AXTree;
}  // namespace ui

class ReadAnythingAppControllerTest;

///////////////////////////////////////////////////////////////////////////////
// ReadAnythingAppController
//
//  A class that controls the Read Anything WebUI app. It serves two purposes:
//  1. Communicate with ReadAnythingPageHandler (written in c++) via mojom.
//  2. Communicate with ReadAnythingApp (written in ts) via gin bindings.
//  The ReadAnythingAppController unserializes the AXTreeUpdate and exposes
//  methods on it to the ts resource for accessing information about the AXTree.
//  This class is owned by the ChromeRenderFrameObserver and has the same
//  lifetime as the render frame.
//
class ReadAnythingAppController
    : public gin::Wrappable<ReadAnythingAppController>,
      public read_anything::mojom::Page {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ReadAnythingAppController(const ReadAnythingAppController&) = delete;
  ReadAnythingAppController& operator=(const ReadAnythingAppController&) =
      delete;

  // Installs v8 context for Read Anything and adds chrome.readAnything binding
  // to page.
  static ReadAnythingAppController* Install(content::RenderFrame* render_frame);

 private:
  friend ReadAnythingAppControllerTest;

  explicit ReadAnythingAppController(content::RenderFrame* render_frame);
  ~ReadAnythingAppController() override;

  // gin::WrappableBase:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // read_anything::mojom::Page:
  void OnAXTreeDistilled(
      const ui::AXTreeUpdate& snapshot,
      const std::vector<ui::AXNodeID>& content_node_ids) override;
  void OnThemeChanged(
      read_anything::mojom::ReadAnythingThemePtr new_theme) override;

  // gin templates:
  std::vector<ui::AXNodeID> DisplayNodeIds();
  SkColor BackgroundColor();
  std::string FontName();
  float FontSize();
  SkColor ForegroundColor();
  float LetterSpacing();
  float LineSpacing();
  std::vector<ui::AXNodeID> GetChildren(ui::AXNodeID ax_node_id);
  std::string GetHtmlTag(ui::AXNodeID ax_node_id);
  std::string GetLanguage(ui::AXNodeID ax_node_id);
  std::string GetTextContent(ui::AXNodeID ax_node_id);
  std::string GetUrl(ui::AXNodeID ax_node_id);
  void OnConnected();

  // The following methods are used for testing ReadAnythingAppTest.
  // Snapshot_lite is a data structure which resembles an AXTreeUpdate. E.g.:
  //   const axTree = {
  //     root_id: 1,
  //     nodes: [
  //       {
  //         id: 1,
  //         role: 'rootWebArea',
  //         child_ids: [2],
  //       },
  //       {
  //         id: 2,
  //         role: 'staticText',
  //         name: 'Some text.',
  //       },
  //     ],
  //   };
  void SetContentForTesting(v8::Local<v8::Value> v8_snapshot_lite,
                            std::vector<ui::AXNodeID> content_node_ids);
  void SetThemeForTesting(const std::string& font_name,
                          float font_size,
                          SkColor foreground_color,
                          SkColor background_color,
                          int line_spacing,
                          int letter_spacing);
  double GetLetterSpacingValue(read_anything::mojom::Spacing letter_spacing);
  double GetLineSpacingValue(read_anything::mojom::Spacing line_spacing);

  ui::AXNode* GetAXNode(ui::AXNodeID ax_node_id);

  // Returns whether the node is part of the selection. Returns true for partial
  // containment as well; it also returns true if part of the node is part of
  // the selection (e.g. a node in which some children are part of the selection
  // and others are not).
  bool SelectionContainsNode(ui::AXNode* ax_node);

  bool NodeIsContentNode(ui::AXNodeID ax_node_id);

  content::RenderFrame* render_frame_;
  mojo::Remote<read_anything::mojom::PageHandlerFactory> page_handler_factory_;
  mojo::Remote<read_anything::mojom::PageHandler> page_handler_;
  mojo::Receiver<read_anything::mojom::Page> receiver_{this};

  // State
  std::unique_ptr<ui::AXTree> tree_;
  std::vector<ui::AXNodeID> content_node_ids_;
  std::vector<ui::AXNodeID> selection_node_ids_;
  bool has_selection_ = false;
  ui::AXNode* start_node_ = nullptr;
  ui::AXNode* end_node_ = nullptr;
  int32_t start_offset_ = -1;
  int32_t end_offset_ = -1;

  SkColor background_color_;
  std::string font_name_;
  float font_size_;
  SkColor foreground_color_;
  float letter_spacing_;
  float line_spacing_;
};

#endif  // CHROME_RENDERER_ACCESSIBILITY_READ_ANYTHING_APP_CONTROLLER_H_
