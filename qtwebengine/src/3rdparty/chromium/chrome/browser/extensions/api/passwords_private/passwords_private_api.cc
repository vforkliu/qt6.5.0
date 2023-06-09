// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/passwords_private/passwords_private_api.h"

#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/passwords_private.h"
#include "components/password_manager/core/browser/manage_passwords_referrer.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/sync/driver/sync_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_function_registry.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace extensions {

namespace {

using ResponseAction = ExtensionFunction::ResponseAction;

PasswordsPrivateDelegate* GetDelegate(
    content::BrowserContext* browser_context) {
  return PasswordsPrivateDelegateFactory::GetForBrowserContext(browser_context,
                                                               /*create=*/true);
}

}  // namespace

// PasswordsPrivateRecordPasswordsPageAccessInSettingsFunction
ResponseAction
PasswordsPrivateRecordPasswordsPageAccessInSettingsFunction::Run() {
  UMA_HISTOGRAM_ENUMERATION(
      "PasswordManager.ManagePasswordsReferrer",
      password_manager::ManagePasswordsReferrer::kChromeSettings);
  return RespondNow(NoArguments());
}

// PasswordsPrivateChangeSavedPasswordFunction
ResponseAction PasswordsPrivateChangeSavedPasswordFunction::Run() {
  auto parameters =
      api::passwords_private::ChangeSavedPassword::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  auto new_id = GetDelegate(browser_context())
                    ->ChangeSavedPassword(parameters->id, parameters->params);
  if (new_id.has_value()) {
    return RespondNow(ArgumentList(
        api::passwords_private::ChangeSavedPassword::Results::Create(
            new_id.value())));
  }
  return RespondNow(Error(
      "Could not change the password. Either the password is empty, the user "
      "is not authenticated or no matching password could be found for the "
      "id."));
}

// PasswordsPrivateRemoveSavedPasswordFunction
ResponseAction PasswordsPrivateRemoveSavedPasswordFunction::Run() {
  auto parameters =
      api::passwords_private::RemoveSavedPassword::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);
  GetDelegate(browser_context())
      ->RemoveSavedPassword(parameters->id, parameters->from_stores);
  return RespondNow(NoArguments());
}

// PasswordsPrivateRemovePasswordExceptionFunction
ResponseAction PasswordsPrivateRemovePasswordExceptionFunction::Run() {
  auto parameters =
      api::passwords_private::RemovePasswordException::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);
  GetDelegate(browser_context())->RemovePasswordException(parameters->id);
  return RespondNow(NoArguments());
}

// PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction
ResponseAction
PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction::Run() {
  GetDelegate(browser_context())->UndoRemoveSavedPasswordOrException();
  return RespondNow(NoArguments());
}

// PasswordsPrivateRequestPlaintextPasswordFunction
ResponseAction PasswordsPrivateRequestPlaintextPasswordFunction::Run() {
  auto parameters =
      api::passwords_private::RequestPlaintextPassword::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  GetDelegate(browser_context())
      ->RequestPlaintextPassword(
          parameters->id, parameters->reason,
          base::BindOnce(
              &PasswordsPrivateRequestPlaintextPasswordFunction::GotPassword,
              this),
          GetSenderWebContents());

  // GotPassword() might respond before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateRequestPlaintextPasswordFunction::GotPassword(
    absl::optional<std::u16string> password) {
  if (password) {
    Respond(OneArgument(base::Value(std::move(*password))));
    return;
  }

  Respond(Error(base::StringPrintf(
      "Could not obtain plaintext password. Either the user is not "
      "authenticated or no password with id = %d could be found.",
      api::passwords_private::RequestPlaintextPassword::Params::Create(args())
          ->id)));
}

// PasswordsPrivateRequestCredentialDetailsFunction
ResponseAction PasswordsPrivateRequestCredentialDetailsFunction::Run() {
  auto parameters =
      api::passwords_private::RequestCredentialDetails::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  GetDelegate(browser_context())
      ->RequestCredentialDetails(
          parameters->id,
          base::BindOnce(&PasswordsPrivateRequestCredentialDetailsFunction::
                             GotPasswordUiEntry,
                         this),
          GetSenderWebContents());

  // GotPasswordUiEntry() might have responded before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateRequestCredentialDetailsFunction::GotPasswordUiEntry(
    absl::optional<api::passwords_private::PasswordUiEntry> password_ui_entry) {
  if (password_ui_entry) {
    Respond(ArgumentList(
        api::passwords_private::RequestCredentialDetails::Results::Create(
            std::move(*password_ui_entry))));
    return;
  }

  Respond(Error(base::StringPrintf(
      "Could not obtain password entry. Either the user is not "
      "authenticated or no credential with id = %d could be found.",
      api::passwords_private::RequestCredentialDetails::Params::Create(args())
          ->id)));
}

// PasswordsPrivateGetSavedPasswordListFunction
ResponseAction PasswordsPrivateGetSavedPasswordListFunction::Run() {
  // GetList() can immediately call GotList() (which would Respond() before
  // RespondLater()). So we post a task to preserve order.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PasswordsPrivateGetSavedPasswordListFunction::GetList,
                     this));
  return RespondLater();
}

void PasswordsPrivateGetSavedPasswordListFunction::GetList() {
  GetDelegate(browser_context())
      ->GetSavedPasswordsList(base::BindOnce(
          &PasswordsPrivateGetSavedPasswordListFunction::GotList, this));
}

void PasswordsPrivateGetSavedPasswordListFunction::GotList(
    const PasswordsPrivateDelegate::UiEntries& list) {
  Respond(ArgumentList(
      api::passwords_private::GetSavedPasswordList::Results::Create(list)));
}

// PasswordsPrivateGetPasswordExceptionListFunction
ResponseAction PasswordsPrivateGetPasswordExceptionListFunction::Run() {
  // GetList() can immediately call GotList() (which would Respond() before
  // RespondLater()). So we post a task to preserve order.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PasswordsPrivateGetPasswordExceptionListFunction::GetList,
                     this));
  return RespondLater();
}

void PasswordsPrivateGetPasswordExceptionListFunction::GetList() {
  GetDelegate(browser_context())
      ->GetPasswordExceptionsList(base::BindOnce(
          &PasswordsPrivateGetPasswordExceptionListFunction::GotList, this));
}

void PasswordsPrivateGetPasswordExceptionListFunction::GotList(
    const PasswordsPrivateDelegate::ExceptionEntries& entries) {
  Respond(ArgumentList(
      api::passwords_private::GetPasswordExceptionList::Results::Create(
          entries)));
}

// PasswordsPrivateMovePasswordToAccountFunction
ResponseAction PasswordsPrivateMovePasswordsToAccountFunction::Run() {
  auto parameters =
      api::passwords_private::MovePasswordsToAccount::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);
  GetDelegate(browser_context())
      ->MovePasswordsToAccount(parameters->ids, GetSenderWebContents());
  return RespondNow(NoArguments());
}

// PasswordsPrivateImportPasswordsFunction
ResponseAction PasswordsPrivateImportPasswordsFunction::Run() {
  auto parameters =
      api::passwords_private::ImportPasswords::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);
  GetDelegate(browser_context())
      ->ImportPasswords(
          parameters->to_store,
          base::BindOnce(
              &PasswordsPrivateImportPasswordsFunction::ImportRequestCompleted,
              this),
          GetSenderWebContents());

  // `ImportRequestCompleted()` might respond before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateImportPasswordsFunction::ImportRequestCompleted(
    const api::passwords_private::ImportResults& result) {
  Respond(ArgumentList(
      api::passwords_private::ImportPasswords::Results::Create(result)));
}

// PasswordsPrivateExportPasswordsFunction
ResponseAction PasswordsPrivateExportPasswordsFunction::Run() {
  GetDelegate(browser_context())
      ->ExportPasswords(
          base::BindOnce(
              &PasswordsPrivateExportPasswordsFunction::ExportRequestCompleted,
              this),
          GetSenderWebContents());
  return RespondLater();
}

void PasswordsPrivateExportPasswordsFunction::ExportRequestCompleted(
    const std::string& error) {
  if (error.empty())
    Respond(NoArguments());
  else
    Respond(Error(error));
}

// PasswordsPrivateCancelExportPasswordsFunction
ResponseAction PasswordsPrivateCancelExportPasswordsFunction::Run() {
  GetDelegate(browser_context())->CancelExportPasswords();
  return RespondNow(NoArguments());
}

// PasswordsPrivateRequestExportProgressStatusFunction
ResponseAction PasswordsPrivateRequestExportProgressStatusFunction::Run() {
  return RespondNow(ArgumentList(
      api::passwords_private::RequestExportProgressStatus::Results::Create(
          GetDelegate(browser_context())->GetExportProgressStatus())));
}

// PasswordsPrivateIsOptedInForAccountStorageFunction
ResponseAction PasswordsPrivateIsOptedInForAccountStorageFunction::Run() {
  return RespondNow(OneArgument(base::Value(
      GetDelegate(browser_context())->IsOptedInForAccountStorage())));
}

// PasswordsPrivateOptInForAccountStorageFunction
ResponseAction PasswordsPrivateOptInForAccountStorageFunction::Run() {
  auto parameters =
      api::passwords_private::OptInForAccountStorage::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters.get());

  GetDelegate(browser_context())
      ->SetAccountStorageOptIn(parameters->opt_in, GetSenderWebContents());
  return RespondNow(NoArguments());
}

// PasswordsPrivateGetInsecureCredentialsFunction:
PasswordsPrivateGetInsecureCredentialsFunction::
    ~PasswordsPrivateGetInsecureCredentialsFunction() = default;

ResponseAction PasswordsPrivateGetInsecureCredentialsFunction::Run() {
  return RespondNow(ArgumentList(
      api::passwords_private::GetInsecureCredentials::Results::Create(
          GetDelegate(browser_context())->GetInsecureCredentials())));
}

// PasswordsPrivateMuteInsecureCredentialFunction:
PasswordsPrivateMuteInsecureCredentialFunction::
    ~PasswordsPrivateMuteInsecureCredentialFunction() = default;

ResponseAction PasswordsPrivateMuteInsecureCredentialFunction::Run() {
  auto parameters =
      api::passwords_private::MuteInsecureCredential::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  if (!GetDelegate(browser_context())
           ->MuteInsecureCredential(parameters->credential)) {
    return RespondNow(
        Error("Could not mute the insecure credential. Probably no matching "
              "password could be found."));
  }

  return RespondNow(NoArguments());
}

// PasswordsPrivateUnmuteInsecureCredentialFunction:
PasswordsPrivateUnmuteInsecureCredentialFunction::
    ~PasswordsPrivateUnmuteInsecureCredentialFunction() = default;

ResponseAction PasswordsPrivateUnmuteInsecureCredentialFunction::Run() {
  auto parameters =
      api::passwords_private::UnmuteInsecureCredential::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  if (!GetDelegate(browser_context())
           ->UnmuteInsecureCredential(parameters->credential)) {
    return RespondNow(
        Error("Could not unmute the insecure credential. Probably no matching "
              "password could be found."));
  }

  return RespondNow(NoArguments());
}

// PasswordsPrivateRecordChangePasswordFlowStartedFunction:
PasswordsPrivateRecordChangePasswordFlowStartedFunction::
    ~PasswordsPrivateRecordChangePasswordFlowStartedFunction() = default;

ResponseAction PasswordsPrivateRecordChangePasswordFlowStartedFunction::Run() {
  auto parameters =
      api::passwords_private::RecordChangePasswordFlowStarted::Params::Create(
          args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  GetDelegate(browser_context())
      ->RecordChangePasswordFlowStarted(parameters->credential,
                                        parameters->is_manual_flow);
  return RespondNow(NoArguments());
}

// PasswordsPrivateRefreshScriptsIfNecessaryFunction:
PasswordsPrivateRefreshScriptsIfNecessaryFunction::
    ~PasswordsPrivateRefreshScriptsIfNecessaryFunction() = default;

ResponseAction PasswordsPrivateRefreshScriptsIfNecessaryFunction::Run() {
  GetDelegate(browser_context())
      ->RefreshScriptsIfNecessary(base::BindOnce(
          &PasswordsPrivateRefreshScriptsIfNecessaryFunction::OnRefreshed,
          base::RetainedRef(this)));

  // OnRefreshed() might respond before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateRefreshScriptsIfNecessaryFunction::OnRefreshed() {
  Respond(NoArguments());
}

// PasswordsPrivateStartPasswordCheckFunction:
PasswordsPrivateStartPasswordCheckFunction::
    ~PasswordsPrivateStartPasswordCheckFunction() = default;

ResponseAction PasswordsPrivateStartPasswordCheckFunction::Run() {
  GetDelegate(browser_context())
      ->StartPasswordCheck(base::BindOnce(
          &PasswordsPrivateStartPasswordCheckFunction::OnStarted, this));

  // OnStarted() might respond before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateStartPasswordCheckFunction::OnStarted(
    password_manager::BulkLeakCheckService::State state) {
  const bool is_running =
      state == password_manager::BulkLeakCheckService::State::kRunning;
  Respond(is_running ? NoArguments()
                     : Error("Starting password check failed."));
}

// PasswordsPrivateStopPasswordCheckFunction:
PasswordsPrivateStopPasswordCheckFunction::
    ~PasswordsPrivateStopPasswordCheckFunction() = default;

ResponseAction PasswordsPrivateStopPasswordCheckFunction::Run() {
  GetDelegate(browser_context())->StopPasswordCheck();
  return RespondNow(NoArguments());
}

// PasswordsPrivateGetPasswordCheckStatusFunction:
PasswordsPrivateGetPasswordCheckStatusFunction::
    ~PasswordsPrivateGetPasswordCheckStatusFunction() = default;

ResponseAction PasswordsPrivateGetPasswordCheckStatusFunction::Run() {
  return RespondNow(ArgumentList(
      api::passwords_private::GetPasswordCheckStatus::Results::Create(
          GetDelegate(browser_context())->GetPasswordCheckStatus())));
}

// PasswordsPrivateStartAutomatedPasswordChangeFunction:
PasswordsPrivateStartAutomatedPasswordChangeFunction::
    ~PasswordsPrivateStartAutomatedPasswordChangeFunction() = default;

ResponseAction PasswordsPrivateStartAutomatedPasswordChangeFunction::Run() {
  auto parameters =
      api::passwords_private::StartAutomatedPasswordChange::Params::Create(
          args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  // Forward the call to the delegate.
  GetDelegate(browser_context())
      ->StartAutomatedPasswordChange(
          parameters->credential,
          base::BindOnce(&PasswordsPrivateStartAutomatedPasswordChangeFunction::
                             OnResultReceived,
                         base::RetainedRef(this)));

  // `OnResultReceived()` might respond before we reach this point.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PasswordsPrivateStartAutomatedPasswordChangeFunction::OnResultReceived(
    bool success) {
  Respond(ArgumentList(
      api::passwords_private::StartAutomatedPasswordChange::Results::Create(
          success)));
}

// PasswordsPrivateIsAccountStoreDefaultFunction
ResponseAction PasswordsPrivateIsAccountStoreDefaultFunction::Run() {
  return RespondNow(OneArgument(
      base::Value(GetDelegate(browser_context())
                      ->IsAccountStoreDefault(GetSenderWebContents()))));
}

// PasswordsPrivateGetUrlCollectionFunction:
ResponseAction PasswordsPrivateGetUrlCollectionFunction::Run() {
  auto parameters =
      api::passwords_private::GetUrlCollection::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  const absl::optional<api::passwords_private::UrlCollection> url_collection =
      GetDelegate(browser_context())->GetUrlCollection(parameters->url);
  if (!url_collection) {
    return RespondNow(
        Error("Provided string doesn't meet password URL requirements. Either "
              "the format is invalid or the scheme is not unsupported."));
  }

  return RespondNow(
      ArgumentList(api::passwords_private::GetUrlCollection::Results::Create(
          url_collection.value())));
}

// PasswordsPrivateAddPasswordFunction
ResponseAction PasswordsPrivateAddPasswordFunction::Run() {
  auto parameters = api::passwords_private::AddPassword::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(parameters);

  if (!GetDelegate(browser_context())
           ->AddPassword(parameters->options.url,
                         base::UTF8ToUTF16(parameters->options.username),
                         base::UTF8ToUTF16(parameters->options.password),
                         base::UTF8ToUTF16(parameters->options.note),
                         parameters->options.use_account_store,
                         GetSenderWebContents())) {
    return RespondNow(Error(
        "Could not add the password. Either the url is invalid, the password "
        "is empty or an entry with such origin and username already exists."));
  }

  return RespondNow(NoArguments());
}

// PasswordsPrivateExtendAuthValidityFunction
ResponseAction PasswordsPrivateExtendAuthValidityFunction::Run() {
  GetDelegate(browser_context())->ExtendAuthValidity();
  return RespondNow(NoArguments());
}

// PasswordsPrivateSwitchBiometricAuthBeforeFillingStateFunction
ResponseAction
PasswordsPrivateSwitchBiometricAuthBeforeFillingStateFunction::Run() {
  GetDelegate(browser_context())
      ->SwitchBiometricAuthBeforeFillingState(GetSenderWebContents());
  return RespondNow(NoArguments());
}

}  // namespace extensions
