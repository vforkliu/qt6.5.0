// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace ash {
class RecommendAppsScreen;
}

namespace chromeos {

// Interface for dependency injection between RecommendAppsScreen and its
// WebUI representation.
class RecommendAppsScreenView
    : public base::SupportsWeakPtr<RecommendAppsScreenView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{"recommend-apps",
                                                       "RecommendAppsScreen"};

  virtual ~RecommendAppsScreenView() = default;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

  // Called when the download of the recommend app list is successful. Shows the
  // downloaded `app_list` to the user.
  virtual void OnLoadSuccess(base::Value app_list) = 0;

  // Called when parsing the recommend app list response fails. Should skip this
  // screen.
  virtual void OnParseResponseError() = 0;
};

// The sole implementation of the RecommendAppsScreenView, using WebUI.
class RecommendAppsScreenHandler : public BaseScreenHandler,
                                   public RecommendAppsScreenView {
 public:
  using TView = RecommendAppsScreenView;

  RecommendAppsScreenHandler();

  RecommendAppsScreenHandler(const RecommendAppsScreenHandler&) = delete;
  RecommendAppsScreenHandler& operator=(const RecommendAppsScreenHandler&) =
      delete;

  ~RecommendAppsScreenHandler() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void GetAdditionalParameters(base::Value::Dict* dict) override;

  // RecommendAppsScreenView:
  void Show() override;
  void Hide() override;
  void OnLoadSuccess(base::Value app_list) override;
  void OnParseResponseError() override;

 private:
  // Call the JS function to load the list of apps in the WebView.
  void LoadAppListInUI(base::Value app_list);
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::RecommendAppsScreenHandler;
using ::chromeos::RecommendAppsScreenView;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_
