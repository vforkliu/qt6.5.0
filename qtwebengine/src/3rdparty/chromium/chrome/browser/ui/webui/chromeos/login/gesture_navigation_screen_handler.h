// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_GESTURE_NAVIGATION_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_GESTURE_NAVIGATION_SCREEN_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

// Interface between gesture navigation screen and its representation.
class GestureNavigationScreenView
    : public base::SupportsWeakPtr<GestureNavigationScreenView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{
      "gesture-navigation", "GestureNavigationScreen"};

  virtual ~GestureNavigationScreenView() = default;

  virtual void Show() = 0;
};

// WebUI implementation of GestureNavigationScreenView.
class GestureNavigationScreenHandler : public GestureNavigationScreenView,
                                       public BaseScreenHandler {
 public:
  using TView = GestureNavigationScreenView;

  GestureNavigationScreenHandler();
  ~GestureNavigationScreenHandler() override;

  GestureNavigationScreenHandler(const GestureNavigationScreenHandler&) =
      delete;
  GestureNavigationScreenHandler operator=(
      const GestureNavigationScreenHandler&) = delete;

  // GestureNavigationScreenView:
  void Show() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::GestureNavigationScreenHandler;
using ::chromeos::GestureNavigationScreenView;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_GESTURE_NAVIGATION_SCREEN_HANDLER_H_
