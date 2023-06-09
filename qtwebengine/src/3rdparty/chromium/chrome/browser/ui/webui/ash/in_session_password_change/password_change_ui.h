// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_ASH_IN_SESSION_PASSWORD_CHANGE_PASSWORD_CHANGE_UI_H_
#define CHROME_BROWSER_UI_WEBUI_ASH_IN_SESSION_PASSWORD_CHANGE_PASSWORD_CHANGE_UI_H_

#include "chrome/browser/ui/webui/chromeos/system_web_dialog_delegate.h"
#include "ui/web_dialogs/web_dialog_ui.h"

namespace ash {

// For chrome:://password-change
class PasswordChangeUI : public ui::WebDialogUI {
 public:
  explicit PasswordChangeUI(content::WebUI* web_ui);

  PasswordChangeUI(const PasswordChangeUI&) = delete;
  PasswordChangeUI& operator=(const PasswordChangeUI&) = delete;

  ~PasswordChangeUI() override;
};

// For chrome:://confirm-password-change
class ConfirmPasswordChangeUI : public ui::WebDialogUI {
 public:
  explicit ConfirmPasswordChangeUI(content::WebUI* web_ui);

  ConfirmPasswordChangeUI(const ConfirmPasswordChangeUI&) = delete;
  ConfirmPasswordChangeUI& operator=(const ConfirmPasswordChangeUI&) = delete;

  ~ConfirmPasswordChangeUI() override;
};

// For chrome:://urgent-password-expiry-notification
class UrgentPasswordExpiryNotificationUI : public ui::WebDialogUI {
 public:
  explicit UrgentPasswordExpiryNotificationUI(content::WebUI* web_ui);

  UrgentPasswordExpiryNotificationUI(
      const UrgentPasswordExpiryNotificationUI&) = delete;
  UrgentPasswordExpiryNotificationUI& operator=(
      const UrgentPasswordExpiryNotificationUI&) = delete;

  ~UrgentPasswordExpiryNotificationUI() override;
};

}  // namespace ash

#endif  // CHROME_BROWSER_UI_WEBUI_ASH_IN_SESSION_PASSWORD_CHANGE_PASSWORD_CHANGE_UI_H_
