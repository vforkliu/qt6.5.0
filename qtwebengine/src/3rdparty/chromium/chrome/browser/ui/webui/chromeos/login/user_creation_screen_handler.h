// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_CREATION_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_CREATION_SCREEN_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace ash {
class UserCreationScreen;
}

namespace chromeos {

// Interface for dependency injection between UserCreationScreen and its
// WebUI representation.
class UserCreationView : public base::SupportsWeakPtr<UserCreationView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{"user-creation",
                                                       "UserCreationScreen"};

  virtual ~UserCreationView() = default;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  virtual void SetIsBackButtonVisible(bool value) = 0;
};

class UserCreationScreenHandler : public UserCreationView,
                                  public BaseScreenHandler {
 public:
  using TView = UserCreationView;

  UserCreationScreenHandler();

  ~UserCreationScreenHandler() override;

  UserCreationScreenHandler(const UserCreationScreenHandler&) = delete;
  UserCreationScreenHandler& operator=(const UserCreationScreenHandler&) =
      delete;

 private:
  void Show() override;
  void SetIsBackButtonVisible(bool value) override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::UserCreationScreenHandler;
using ::chromeos::UserCreationView;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_USER_CREATION_SCREEN_HANDLER_H_
