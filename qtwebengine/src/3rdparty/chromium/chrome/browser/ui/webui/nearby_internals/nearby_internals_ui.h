// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NEARBY_INTERNALS_NEARBY_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_NEARBY_INTERNALS_NEARBY_INTERNALS_UI_H_

#include "ui/webui/mojo_web_ui_controller.h"

// The WebUI controller for chrome://nearby-internals.
class NearbyInternalsUI : public ui::MojoWebUIController {
 public:
  explicit NearbyInternalsUI(content::WebUI* web_ui);
  NearbyInternalsUI(const NearbyInternalsUI&) = delete;
  NearbyInternalsUI& operator=(const NearbyInternalsUI&) = delete;
  ~NearbyInternalsUI() override;

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // CHROME_BROWSER_UI_WEBUI_NEARBY_INTERNALS_NEARBY_INTERNALS_UI_H_
