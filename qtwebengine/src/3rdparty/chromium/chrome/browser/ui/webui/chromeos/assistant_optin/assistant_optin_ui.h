// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_

#include <vector>

#include "ash/public/cpp/assistant/assistant_setup.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "chrome/browser/ui/webui/chromeos/assistant_optin/assistant_optin_utils.h"
#include "chrome/browser/ui/webui/chromeos/login/assistant_optin_flow_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"
#include "chrome/browser/ui/webui/chromeos/system_web_dialog_delegate.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/web_dialogs/web_dialog_ui.h"

namespace chromeos {

// Controller for chrome://assistant-optin/ page.
class AssistantOptInUI : public ui::WebDialogUI {
 public:
  explicit AssistantOptInUI(content::WebUI* web_ui);

  AssistantOptInUI(const AssistantOptInUI&) = delete;
  AssistantOptInUI& operator=(const AssistantOptInUI&) = delete;

  ~AssistantOptInUI() override;

  // Called when the dialog is closed.
  void OnDialogClosed();

 private:
  AssistantOptInFlowScreenHandler* assistant_handler_ptr_;
  base::WeakPtrFactory<AssistantOptInUI> weak_factory_{this};
};

// Dialog delegate for the assistant optin page.
class AssistantOptInDialog : public SystemWebDialogDelegate {
 public:
  AssistantOptInDialog(const AssistantOptInDialog&) = delete;
  AssistantOptInDialog& operator=(const AssistantOptInDialog&) = delete;

  // Shows the assistant optin dialog.
  static void Show(ash::FlowType type = ash::FlowType::kConsentFlow,
                   ash::AssistantSetup::StartAssistantOptInFlowCallback
                       callback = base::DoNothing());

  // Returns true and bounces the window if the dialog is active.
  static bool BounceIfActive();

 protected:
  AssistantOptInDialog(
      ash::FlowType type,
      ash::AssistantSetup::StartAssistantOptInFlowCallback callback);
  ~AssistantOptInDialog() override;

  // SystemWebDialogDelegate
  void AdjustWidgetInitParams(views::Widget::InitParams* params) override;

  // ui::WebDialogDelegate
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogShown(content::WebUI* webui) override;
  void OnDialogClosed(const std::string& json_retval) override;

 private:
  AssistantOptInUI* assistant_ui_ = nullptr;

  // Callback to run if the flow is completed.
  ash::AssistantSetup::StartAssistantOptInFlowCallback callback_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_
