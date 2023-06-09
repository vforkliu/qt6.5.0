// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/enterprise_platform_keys/enterprise_platform_keys_api.h"

#include <utility>

#include "base/bind.h"
#include "base/containers/span.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "chrome/browser/ash/attestation/mock_tpm_challenge_key.h"
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/ash/platform_keys/key_permissions/fake_user_private_token_kpm_service.h"
#include "chrome/browser/ash/platform_keys/key_permissions/mock_key_permissions_manager.h"
#include "chrome/browser/ash/platform_keys/key_permissions/user_private_token_kpm_service_factory.h"
#include "chrome/browser/extensions/api/enterprise_platform_keys_private/enterprise_platform_keys_private_api.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/signin/public/identity_manager/identity_test_utils.h"
#include "components/user_manager/scoped_user_manager.h"
#include "extensions/common/extension_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Invoke;
using testing::NiceMock;

namespace utils = extension_function_test_utils;

namespace extensions {
namespace {

const char kUserEmail[] = "test@google.com";

void FakeRunCheckNotRegister(ash::attestation::AttestationKeyType key_type,
                             Profile* profile,
                             ash::attestation::TpmChallengeKeyCallback callback,
                             const std::string& challenge,
                             bool register_key,
                             const std::string& key_name_for_spkac,
                             const absl::optional<std::string>& signals) {
  EXPECT_FALSE(register_key);
  std::move(callback).Run(
      ash::attestation::TpmChallengeKeyResult::MakeChallengeResponse(
          "response"));
}

class EPKChallengeKeyTestBase : public BrowserWithTestWindowTest {
 protected:
  EPKChallengeKeyTestBase()
      : extension_(ExtensionBuilder("Test").Build()),
        fake_user_manager_(new ash::FakeChromeUserManager()),
        user_manager_enabler_(base::WrapUnique(fake_user_manager_)) {
    stub_install_attributes_.SetCloudManaged("google.com", "device_id");
  }

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    prefs_ = browser()->profile()->GetPrefs();
    SetAuthenticatedUser();

    // UserPrivateTokenKeyPermissionsManagerService and the underlying
    // KeyPermissionsManager are not actually used by *ChallengeKey* classes,
    // but they are created as a part of KeystoreService, so just fake them out.
    // It is ok to pass an unretained pointer because the factory should only be
    // used during the tests' lifetime.
    ash::platform_keys::UserPrivateTokenKeyPermissionsManagerServiceFactory::
        GetInstance()
            ->SetTestingFactory(
                browser()->profile(),
                base::BindRepeating(&EPKChallengeKeyTestBase::
                                        CreateKeyPermissionsManagerService,
                                    base::Unretained(this)));
  }

  void SetMockTpmChallenger() {
    auto mock_tpm_challenge_key =
        std::make_unique<NiceMock<ash::attestation::MockTpmChallengeKey>>();
    // Will be used with EXPECT_CALL.
    mock_tpm_challenge_key_ = mock_tpm_challenge_key.get();
    mock_tpm_challenge_key->EnableFake();
    // transfer ownership inside factory
    ash::attestation::TpmChallengeKeyFactory::SetForTesting(
        std::move(mock_tpm_challenge_key));
  }

  // This will be called by BrowserWithTestWindowTest::SetUp();
  TestingProfile* CreateProfile() override {
    fake_user_manager_->AddUserWithAffiliation(
        AccountId::FromUserEmail(kUserEmail), true);
    return profile_manager()->CreateTestingProfile(kUserEmail);
  }

  std::unique_ptr<KeyedService> CreateKeyPermissionsManagerService(
      content::BrowserContext* context) {
    return std::make_unique<
        ash::platform_keys::FakeUserPrivateTokenKeyPermissionsManagerService>(
        &key_permissions_manager_);
  }

  // Derived classes can override this method to set the required authenticated
  // user in the IdentityManager class.
  virtual void SetAuthenticatedUser() {
    signin::MakePrimaryAccountAvailable(
        IdentityManagerFactory::GetForProfile(browser()->profile()), kUserEmail,
        signin::ConsentLevel::kSync);
  }

  // Like extension_function_test_utils::RunFunctionAndReturnError but with an
  // explicit ListValue.
  std::string RunFunctionAndReturnError(ExtensionFunction* function,
                                        std::unique_ptr<base::ListValue> args,
                                        Browser* browser) {
    utils::RunFunction(function, std::move(args), browser,
                       extensions::api_test_utils::NONE);
    EXPECT_EQ(ExtensionFunction::FAILED, *function->response_type());
    return function->GetError();
  }

  // Like extension_function_test_utils::RunFunctionAndReturnSingleResult but
  // with an explicit ListValue.
  base::Value RunFunctionAndReturnSingleResult(
      ExtensionFunction* function,
      std::unique_ptr<base::ListValue> args,
      Browser* browser) {
    scoped_refptr<ExtensionFunction> function_owner(function);
    // Without a callback the function will not generate a result.
    function->set_has_callback(true);
    utils::RunFunction(function, std::move(args), browser,
                       extensions::api_test_utils::NONE);
    EXPECT_TRUE(function->GetError().empty())
        << "Unexpected error: " << function->GetError();
    if (function->GetResultList() && !function->GetResultList()->empty()) {
      return (*function->GetResultList())[0].Clone();
    }
    return base::Value();
  }

  scoped_refptr<const extensions::Extension> extension_;
  ash::StubInstallAttributes stub_install_attributes_;
  // fake_user_manager_ is owned by user_manager_enabler_.
  ash::FakeChromeUserManager* fake_user_manager_ = nullptr;
  user_manager::ScopedUserManager user_manager_enabler_;
  ash::platform_keys::MockKeyPermissionsManager key_permissions_manager_;
  PrefService* prefs_ = nullptr;
  ash::attestation::MockTpmChallengeKey* mock_tpm_challenge_key_ = nullptr;
};

class EPKChallengeMachineKeyTest : public EPKChallengeKeyTestBase {
 protected:
  EPKChallengeMachineKeyTest()
      : func_(new EnterprisePlatformKeysChallengeMachineKeyFunction()) {
    func_->set_extension(extension_.get());
  }

  std::unique_ptr<base::ListValue> CreateArgs() {
    return CreateArgsInternal(base::Value());
  }

  std::unique_ptr<base::ListValue> CreateArgsNoRegister() {
    return CreateArgsInternal(base::Value(false));
  }

  std::unique_ptr<base::ListValue> CreateArgsRegister() {
    return CreateArgsInternal(base::Value(true));
  }

  std::unique_ptr<base::ListValue> CreateArgsInternal(
      base::Value register_key) {
    static constexpr base::StringPiece kData = "challenge";
    base::Value args(base::Value::Type::LIST);
    args.Append(base::Value(base::as_bytes(base::make_span(kData))));
    if (register_key.is_bool())
      args.Append(std::move(register_key));
    return base::ListValue::From(
        base::Value::ToUniquePtrValue(std::move(args)));
  }

  scoped_refptr<EnterprisePlatformKeysChallengeMachineKeyFunction> func_;
  base::ListValue args_;
};

TEST_F(EPKChallengeMachineKeyTest, ExtensionNotAllowed) {
  base::ListValue empty_allowlist;
  prefs_->Set(prefs::kAttestationExtensionAllowlist, empty_allowlist);

  EXPECT_EQ(
      ash::attestation::TpmChallengeKeyResult::kExtensionNotAllowedErrorMsg,
      RunFunctionAndReturnError(func_.get(), CreateArgs(), browser()));
}

TEST_F(EPKChallengeMachineKeyTest, Success) {
  SetMockTpmChallenger();

  base::Value allowlist(base::Value::Type::LIST);
  allowlist.Append(extension_->id());
  prefs_->Set(prefs::kAttestationExtensionAllowlist, allowlist);

  base::Value value(
      RunFunctionAndReturnSingleResult(func_.get(), CreateArgs(), browser()));

  ASSERT_TRUE(value.is_blob());
  std::string response(value.GetBlob().begin(), value.GetBlob().end());
  EXPECT_EQ("response", response);
}

TEST_F(EPKChallengeMachineKeyTest, KeyNotRegisteredByDefault) {
  SetMockTpmChallenger();

  base::ListValue allowlist;
  allowlist.Append(extension_->id());
  prefs_->Set(prefs::kAttestationExtensionAllowlist, allowlist);

  EXPECT_CALL(*mock_tpm_challenge_key_, BuildResponse)
      .WillOnce(Invoke(FakeRunCheckNotRegister));

  EXPECT_TRUE(utils::RunFunction(func_.get(), CreateArgs(), browser(),
                                 extensions::api_test_utils::NONE));
}

class EPKChallengeUserKeyTest : public EPKChallengeKeyTestBase {
 protected:
  EPKChallengeUserKeyTest()
      : func_(new EnterprisePlatformKeysChallengeUserKeyFunction()) {
    func_->set_extension(extension_.get());
  }

  void SetUp() override {
    EPKChallengeKeyTestBase::SetUp();

    // Set the user preferences.
    prefs_->SetBoolean(prefs::kAttestationEnabled, true);
  }

  std::unique_ptr<base::ListValue> CreateArgs() {
    return CreateArgsInternal(true);
  }

  std::unique_ptr<base::ListValue> CreateArgsNoRegister() {
    return CreateArgsInternal(false);
  }

  std::unique_ptr<base::ListValue> CreateArgsInternal(bool register_key) {
    static constexpr base::StringPiece kData = "challenge";
    base::Value args(base::Value::Type::LIST);
    args.Append(base::Value(base::as_bytes(base::make_span(kData))));
    args.Append(register_key);
    return base::ListValue::From(
        base::Value::ToUniquePtrValue(std::move(args)));
  }

  EPKPChallengeKey impl_;
  scoped_refptr<EnterprisePlatformKeysChallengeUserKeyFunction> func_;
};

TEST_F(EPKChallengeUserKeyTest, ExtensionNotAllowed) {
  base::ListValue empty_allowlist;
  prefs_->Set(prefs::kAttestationExtensionAllowlist, empty_allowlist);

  EXPECT_EQ(
      ash::attestation::TpmChallengeKeyResult::kExtensionNotAllowedErrorMsg,
      RunFunctionAndReturnError(func_.get(), CreateArgs(), browser()));
}

}  // namespace
}  // namespace extensions
