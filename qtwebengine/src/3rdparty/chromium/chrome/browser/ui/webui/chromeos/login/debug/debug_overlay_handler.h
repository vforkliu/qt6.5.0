// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEBUG_DEBUG_OVERLAY_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEBUG_DEBUG_OVERLAY_HANDLER_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"

namespace chromeos {

class DebugOverlayHandler : public BaseWebUIHandler {
 public:
  DebugOverlayHandler();
  ~DebugOverlayHandler() override;
  DebugOverlayHandler(const DebugOverlayHandler&) = delete;
  DebugOverlayHandler& operator=(const DebugOverlayHandler&) = delete;

  // BaseWebUIHandler:
  void DeclareJSCallbacks() override;
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void InitializeDeprecated() override;

 private:
  // JS callbacks.
  void HandleCaptureScreenshot(const std::string& name);
  void ToggleColorMode();

  base::FilePath screenshot_dir_;
  int screenshot_index_ = 0;
  bool add_resolution_to_filename_ = false;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEBUG_DEBUG_OVERLAY_HANDLER_H_
