// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/signin_helper_chromeos.h"

#include "ash/constants/ash_features.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ash/account_manager/account_apps_availability.h"
#include "chrome/browser/ash/account_manager/account_apps_availability_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/ash/components/account_manager/account_manager_factory.h"
#include "components/account_manager_core/account.h"
#include "components/account_manager_core/chromeos/account_manager.h"
#include "components/account_manager_core/chromeos/account_manager_mojo_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "google_apis/gaia/gaia_urls.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

namespace chromeos {

class SigninHelperChromeOSTest;

namespace {

const char kFakePrimaryGaiaId[] = "primary_account_gaia";
const char kFakePrimaryEmail[] = "primary@example.com";
const char kFakeGaiaId[] = "fake_gaia_id";
const char kFakeEmail[] = "fake_email@gmail.com";
const char kFakeAuthCode[] = "fake_auth_code";
const char kFakeDeviceId[] = "fake_device_id";
const char kFakeRefreshToken[] = "fake_refresh_token";
const char kFakeEnterpriseGaiaId[] = "fake_enterprise_gaia_id";
const char kFakeEnterpriseEmail[] = "fake_enterprise@example.com";
const char kFakeEnterpriseDomain[] = "example.com";

const char kSecureConnectApiGetSecondaryGoogleAccountUsageURL[] =
    "https://secureconnect-pa.clients6.google.com/"
    "v1:getManagedAccountsSigninRestriction?policy_name="
    "SecondaryGoogleAccountUsage";

// Fake responses for the URL requests that are part of the sign-in flow.
const char kOnClientOAuthSuccessBody[] =
    R"({
            "refresh_token": "refresh_token",
            "access_token": "access_token",
            "expires_in": 99999
       })";
const char kUserInfoURLBodyWithHostedDomain[] = R"({"hd": "%s"})";
const char kUserInfoURLBodyWithoutHostedDomain[] = R"({})";
const char kSecureConnectApiGetSecondaryGoogleAccountUsageURLBody[] =
    R"({"policyValue": "%s"})";

constexpr char kSecondaryGoogleAccountUsageHistogramName[] =
    "Enterprise.SecondaryGoogleAccountUsage.PolicyFetch.Status";
constexpr char kSecondaryGoogleAccountUsageLatencyHistogramName[] =
    "Enterprise.SecondaryGoogleAccountUsage.PolicyFetch.ResponseLatency";

// Convenience helper to allow a `closure` to be used in a context which is
// expecting a callback with arguments.
template <typename... T>
const base::RepeatingCallback<void(T...)> IgnoreArgs(
    base::RepeatingClosure closure) {
  return closure ? base::BindRepeating([](T...) {}).Then(std::move(closure))
                 : base::BindRepeating([](T...) { NOTREACHED(); });
}

class TestSigninHelper : public SigninHelper {
 public:
  TestSigninHelper(
      SigninHelperChromeOSTest* test_fixture,
      account_manager::AccountManager* account_manager,
      crosapi::AccountManagerMojoService* account_manager_mojo_service,
      const base::RepeatingClosure& close_dialog_closure,
      const base::RepeatingCallback<void(const std::string&,
                                         const std::string&)>&
          show_signin_blocked_by_policy_page,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<ArcHelper> arc_helper,
      const std::string& gaia_id,
      const std::string& email,
      const std::string& auth_code,
      const std::string& signin_scoped_device_id)
      : SigninHelper(account_manager,
                     account_manager_mojo_service,
                     close_dialog_closure,
                     show_signin_blocked_by_policy_page,
                     url_loader_factory,
                     std::move(arc_helper),
                     gaia_id,
                     email,
                     auth_code,
                     signin_scoped_device_id) {
    test_fixture_ = test_fixture;
  }

  ~TestSigninHelper() override;

 private:
  SigninHelperChromeOSTest* test_fixture_;
};

}  // namespace

class SigninHelperChromeOSTest
    : public InProcessBrowserTest,
      public account_manager::AccountManager::Observer {
 public:
  SigninHelperChromeOSTest()
      : test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

  ~SigninHelperChromeOSTest() override {
    DCHECK_EQ(signin_helper_created_count_, signin_helper_deleted_count_);
  }

  void SetUpOnMainThread() override {
    auto* profile = browser()->profile();
    auto* factory =
        g_browser_process->platform_part()->GetAccountManagerFactory();
    account_manager_ = factory->GetAccountManager(profile->GetPath().value());
    account_manager_mojo_service_ =
        factory->GetAccountManagerMojoService(profile->GetPath().value());
    account_manager_->SetUrlLoaderFactoryForTests(shared_url_loader_factory());
    account_manager_->AddObserver(this);

    // Setup the main account:
    account_manager::AccountKey kPrimaryAccountKey{
        kFakePrimaryGaiaId, account_manager::AccountType::kGaia};
    account_manager()->UpsertAccount(kPrimaryAccountKey, kFakePrimaryEmail,
                                     "access_token");
    base::RunLoop().RunUntilIdle();
    on_token_upserted_call_count_ = 0;
    on_token_upserted_account_ = absl::nullopt;
  }

  void TearDownOnMainThread() override {
    account_manager_->RemoveObserver(this);
    on_token_upserted_call_count_ = 0;
    on_token_upserted_account_ = absl::nullopt;
  }

  void CreateSigninHelper(const base::RepeatingClosure& close_dialog_closure) {
    OnSigninHelperCreated();
    new TestSigninHelper(
        this, account_manager(), account_manager_mojo_service(),
        close_dialog_closure,
        /*show_signin_blocked_by_policy_page=*/base::DoNothing(),
        shared_url_loader_factory(),
        /*arc_helper=*/nullptr, kFakeGaiaId, kFakeEmail, kFakeAuthCode,
        kFakeDeviceId);
  }

  GaiaAuthConsumer::ClientOAuthResult GetFakeOAuthResult() {
    return GaiaAuthConsumer::ClientOAuthResult(kFakeRefreshToken, "", 0, false,
                                               false);
  }

  void OnSigninHelperCreated() { ++signin_helper_created_count_; }
  void OnSigninHelperDeleted() { ++signin_helper_deleted_count_; }

  int on_token_upserted_call_count() { return on_token_upserted_call_count_; }

  absl::optional<account_manager::Account> on_token_upserted_account() {
    return on_token_upserted_account_;
  }

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return test_shared_loader_factory_;
  }

  account_manager::AccountManager* account_manager() {
    return account_manager_;
  }

  crosapi::AccountManagerMojoService* account_manager_mojo_service() {
    return account_manager_mojo_service_;
  }

 protected:
  void AddResponseClientOAuthSuccess() {
    loader_factory().AddResponse(
        GaiaUrls::GetInstance()->oauth2_token_url().spec(),
        /*content=*/kOnClientOAuthSuccessBody, net::HTTP_OK);
  }

  void AddResponseClientOAuthFailure() {
    loader_factory().AddResponse(
        GaiaUrls::GetInstance()->oauth2_token_url().spec(),
        /*content=*/R"({})", net::HTTP_BAD_REQUEST);
  }

  void AddResponseGetSecondaryGoogleAccountUsage(
      const std::string& policy_value) {
    loader_factory().AddResponse(
        kSecureConnectApiGetSecondaryGoogleAccountUsageURL,
        /*content=*/
        base::StringPrintf(
            kSecureConnectApiGetSecondaryGoogleAccountUsageURLBody,
            policy_value.c_str()),
        net::HTTP_OK);
  }

  void AddResponseGetUserInfoWithHostedDomain(
      const std::string& hosted_domain) {
    loader_factory().AddResponse(
        GaiaUrls::GetInstance()->oauth_user_info_url().spec(),
        /*content=*/
        base::StringPrintf(kUserInfoURLBodyWithHostedDomain,
                           hosted_domain.c_str()),
        net::HTTP_OK);
  }

  void AddResponseGetUserInfoWithoutHostedDomain() {
    loader_factory().AddResponse(
        GaiaUrls::GetInstance()->oauth_user_info_url().spec(),
        /*content=*/kUserInfoURLBodyWithoutHostedDomain, net::HTTP_OK);
  }

  void AddResponseRevokeGaiaTokenOnServer() {
    loader_factory().AddResponse(
        GaiaUrls::GetInstance()->oauth2_revoke_url().spec(),
        /*content=*/std::string(), net::HTTP_OK);
  }

  network::TestURLLoaderFactory& loader_factory() {
    return test_url_loader_factory_;
  }

 private:
  // account_manager::AccountManager::Observer overrides:
  void OnTokenUpserted(const account_manager::Account& account) override {
    ++on_token_upserted_call_count_;
    on_token_upserted_account_ = account;
  }

  void OnAccountRemoved(const account_manager::Account& account) override {}

  account_manager::AccountManager* account_manager_ = nullptr;
  crosapi::AccountManagerMojoService* account_manager_mojo_service_ = nullptr;
  int signin_helper_created_count_ = 0;
  int signin_helper_deleted_count_ = 0;
  int on_token_upserted_call_count_ = 0;
  absl::optional<account_manager::Account> on_token_upserted_account_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
};

TestSigninHelper::~TestSigninHelper() {
  test_fixture_->OnSigninHelperDeleted();
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTest,
                       NoAccountAddedWhenAuthTokenFetchFails) {
  base::RunLoop close_dialog_closure_run_loop;
  // Set auth token fetch to fail.
  AddResponseClientOAuthFailure();
  CreateSigninHelper(
      base::BindLambdaForTesting([&close_dialog_closure_run_loop]() {
        close_dialog_closure_run_loop.Quit();
      }));
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();
  // No account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 0);
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTest,
                       AccountAddedWhenAuthTokenFetchSucceeds) {
  base::RunLoop close_dialog_closure_run_loop;
  CreateSigninHelper(close_dialog_closure_run_loop.QuitClosure());
  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();
  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);
  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEmail);
}

class SigninHelperChromeOSTestWithArcAccountRestrictions
    : public SigninHelperChromeOSTest,
      public ::ash::AccountAppsAvailability::Observer {
 public:
  SigninHelperChromeOSTestWithArcAccountRestrictions() {
    feature_list_.InitAndEnableFeature(chromeos::features::kLacrosSupport);
  }

  ~SigninHelperChromeOSTestWithArcAccountRestrictions() override = default;

  void SetUpOnMainThread() override {
    SigninHelperChromeOSTest::SetUpOnMainThread();
    account_apps_availability_ =
        ash::AccountAppsAvailabilityFactory::GetForProfile(
            browser()->profile());
    // In-session account addition happens when `AccountAppsAvailability` is
    // already initialized.
    EXPECT_TRUE(account_apps_availability()->IsInitialized());
    account_apps_availability()->AddObserver(this);
  }

  void TearDownOnMainThread() override {
    account_apps_availability()->RemoveObserver(this);
    on_account_available_in_arc_call_count_ = 0;
    on_account_unavailable_in_arc_call_count_ = 0;
    on_account_available_in_arc_account_ = absl::nullopt;
    on_account_unavailable_in_arc_account_ = absl::nullopt;
    SigninHelperChromeOSTest::TearDownOnMainThread();
  }

  void CreateSigninHelper(std::unique_ptr<SigninHelper::ArcHelper> arc_helper,
                          const base::RepeatingClosure& close_dialog_closure) {
    OnSigninHelperCreated();
    new TestSigninHelper(
        this, account_manager(), account_manager_mojo_service(),
        close_dialog_closure,
        /*show_signin_blocked_by_policy_page=*/base::DoNothing(),
        shared_url_loader_factory(), std::move(arc_helper), kFakeGaiaId,
        kFakeEmail, kFakeAuthCode, kFakeDeviceId);
  }

  bool IsAccountAvailableInArc(account_manager::Account account) {
    bool result = false;
    base::RunLoop run_loop;
    account_apps_availability()->GetAccountsAvailableInArc(
        base::BindLambdaForTesting(
            [&result, &account, &run_loop](
                const base::flat_set<account_manager::Account>& accounts) {
              for (const auto& a : accounts) {
                if (a.raw_email == account.raw_email)
                  result = true;
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return result;
  }

  ash::AccountAppsAvailability* account_apps_availability() {
    return account_apps_availability_;
  }

  int on_account_available_in_arc_call_count() {
    return on_account_available_in_arc_call_count_;
  }

  int on_account_unavailable_in_arc_call_count() {
    return on_account_unavailable_in_arc_call_count_;
  }

  absl::optional<account_manager::Account>
  on_account_available_in_arc_account() {
    return on_account_available_in_arc_account_;
  }

  absl::optional<account_manager::Account>
  on_account_unavailable_in_arc_account() {
    return on_account_unavailable_in_arc_account_;
  }

 private:
  void OnAccountAvailableInArc(
      const account_manager::Account& account) override {
    ++on_account_available_in_arc_call_count_;
    on_account_available_in_arc_account_ = account;
  }

  void OnAccountUnavailableInArc(
      const account_manager::Account& account) override {
    ++on_account_unavailable_in_arc_call_count_;
    on_account_unavailable_in_arc_account_ = account;
  }

  int on_account_available_in_arc_call_count_ = 0;
  int on_account_unavailable_in_arc_call_count_ = 0;
  absl::optional<account_manager::Account> on_account_available_in_arc_account_;
  absl::optional<account_manager::Account>
      on_account_unavailable_in_arc_account_;
  ash::AccountAppsAvailability* account_apps_availability_;
  base::test::ScopedFeatureList feature_list_;
};

// Account is available in ARC after account addition if `is_available_in_arc`
// is set to `true`.
IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestWithArcAccountRestrictions,
                       AccountIsAvailableInArcAfterAddition) {
  std::unique_ptr<SigninHelper::ArcHelper> arc_helper =
      std::make_unique<SigninHelper::ArcHelper>(
          /*is_available_in_arc=*/true, /*is_account_addition=*/true,
          account_apps_availability());
  base::RunLoop close_dialog_closure_run_loop;
  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  CreateSigninHelper(std::move(arc_helper),
                     close_dialog_closure_run_loop.QuitClosure());
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();
  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);
  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEmail);
  // 0 account should be available in ARC.
  EXPECT_EQ(on_account_unavailable_in_arc_call_count(), 0);
  // Note: after we receive one `OnAccountAvailableInArc` call - we may get
  // another call after the refresh token is updated for account.
  EXPECT_GT(on_account_available_in_arc_call_count(), 0);
  auto arc_account = on_account_available_in_arc_account();
  ASSERT_TRUE(arc_account.has_value());
  EXPECT_EQ(arc_account.value().raw_email, kFakeEmail);
  // `AccountAppsAvailability::GetAccountsAvailableInArc` should return account
  // list containing this account.
  EXPECT_TRUE(IsAccountAvailableInArc(account.value()));
}

// Account is not available in ARC after account addition if
// `is_available_in_arc` is set to `false`.
IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestWithArcAccountRestrictions,
                       AccountIsNotAvailableInArcAfterAddition) {
  std::unique_ptr<SigninHelper::ArcHelper> arc_helper =
      std::make_unique<SigninHelper::ArcHelper>(
          /*is_available_in_arc=*/false, /*is_account_addition=*/true,
          account_apps_availability());
  base::RunLoop close_dialog_closure_run_loop;
  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  CreateSigninHelper(
      std::move(arc_helper),
      base::BindLambdaForTesting([&close_dialog_closure_run_loop]() {
        close_dialog_closure_run_loop.Quit();
      }));
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();
  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);
  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEmail);

  // The account didn't exist (and therefore wasn't available in ARC) before, so
  // no `OnAccountUnavailableInArc` calls are expected.
  EXPECT_EQ(on_account_available_in_arc_call_count(), 0);
  EXPECT_EQ(on_account_unavailable_in_arc_call_count(), 0);
  // `AccountAppsAvailability::GetAccountsAvailableInArc` should return account
  // list not containing this account.
  EXPECT_FALSE(IsAccountAvailableInArc(account.value()));
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestWithArcAccountRestrictions,
                       ArcAvailabilityIsNotSetAfterReauthentication) {
  account_manager::AccountKey kAccountKey{kFakeGaiaId,
                                          account_manager::AccountType::kGaia};
  account_manager()->UpsertAccount(kAccountKey, kFakeEmail, "access_token");
  base::RunLoop().RunUntilIdle();
  // 1 account should be added.
  const int initial_upserted_calls = 1;
  EXPECT_EQ(on_token_upserted_call_count(), initial_upserted_calls);

  // Go through a reauthentication flow.
  std::unique_ptr<SigninHelper::ArcHelper> arc_helper =
      std::make_unique<SigninHelper::ArcHelper>(
          /*is_available_in_arc=*/true, /*is_account_addition=*/false,
          account_apps_availability());
  base::RunLoop close_dialog_closure_run_loop;
  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  CreateSigninHelper(
      std::move(arc_helper),
      base::BindLambdaForTesting([&close_dialog_closure_run_loop]() {
        close_dialog_closure_run_loop.Quit();
      }));
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();
  // 1 account should be updated.
  EXPECT_EQ(on_token_upserted_call_count(), initial_upserted_calls + 1);
  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEmail);
  // 0 accounts should be added as "available in ARC".
  EXPECT_EQ(on_account_available_in_arc_call_count(), 0);
  EXPECT_EQ(on_account_unavailable_in_arc_call_count(), 0);
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestWithArcAccountRestrictions,
                       AccountAvailabilityDoesntChangeAfterReauthentication) {}

class SigninHelperChromeOSTestSecondaryGoogleAccountUsage
    : public SigninHelperChromeOSTest {
 public:
  SigninHelperChromeOSTestSecondaryGoogleAccountUsage() {
    feature_list_.InitWithFeatures(
        /*enabled_features=*/{chromeos::features::kSecondaryGoogleAccountUsage,
                              chromeos::features::kLacrosSupport},
        /*disabled_features=*/{chromeos::features::kLacrosSupport});
  }

  ~SigninHelperChromeOSTestSecondaryGoogleAccountUsage() override = default;

  void CreateSigninHelper(
      const base::RepeatingClosure& close_dialog_closure,
      const base::RepeatingClosure& show_signin_blocked_by_policy_page,
      const std::string& gaia_id,
      const std::string& email) {
    OnSigninHelperCreated();
    // The `TestSigninHelper` deletes itself after its work is complete.

    new TestSigninHelper(this, account_manager(),
                         account_manager_mojo_service(),
                         /*close_dialog_closure=*/close_dialog_closure,
                         /*show_signin_blocked_by_policy_page=*/
                         IgnoreArgs<const std::string&, const std::string&>(
                             show_signin_blocked_by_policy_page),
                         shared_url_loader_factory(), /*arc_helper=*/nullptr,
                         gaia_id, email, kFakeAuthCode, kFakeDeviceId);
  }

 protected:
  base::HistogramTester histogram_tester_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestSecondaryGoogleAccountUsage,
                       AccountAddedForNonEnterpriseAccount) {
  base::RunLoop close_dialog_closure_run_loop;

  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  // Set no hosted domain for user info request.
  AddResponseGetUserInfoWithoutHostedDomain();
  // Non Enterprise account tries to sign in.
  CreateSigninHelper(
      /*close_dialog_closure=*/close_dialog_closure_run_loop.QuitClosure(),
      /*show_signin_blocked_by_policy_page=*/
      base::RepeatingClosure(), kFakeGaiaId, kFakeEmail);

  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();

  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);

  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEmail);
  histogram_tester_.ExpectBucketCount(
      kSecondaryGoogleAccountUsageHistogramName,
      ash::UserCloudSigninRestrictionPolicyFetcherChromeOS::Status::
          kUnsupportedAccountTypeError,
      1);
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestSecondaryGoogleAccountUsage,
                       AccountAddedForEnterpriseAccountWithNoPolicySet) {
  base::RunLoop close_dialog_closure_run_loop;

  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  // Set user info response with hosted domain (hd) value.
  AddResponseGetUserInfoWithHostedDomain(kFakeEnterpriseDomain);
  // Set SecondaryGoogleAccountUsage policy fetch to unset.
  AddResponseGetSecondaryGoogleAccountUsage("unset");
  // Enterprise account tries to sign in.
  CreateSigninHelper(
      /*close_dialog_closure=*/close_dialog_closure_run_loop.QuitClosure(),
      /*show_signin_blocked_by_policy_page=*/
      base::RepeatingClosure(), kFakeEnterpriseGaiaId, kFakeEnterpriseEmail);
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();

  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);

  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEnterpriseEmail);
  histogram_tester_.ExpectBucketCount(
      kSecondaryGoogleAccountUsageHistogramName,
      ash::UserCloudSigninRestrictionPolicyFetcherChromeOS::Status::kSuccess,
      1);
  histogram_tester_.ExpectTotalCount(
      kSecondaryGoogleAccountUsageLatencyHistogramName, 1);
}

IN_PROC_BROWSER_TEST_F(
    SigninHelperChromeOSTestSecondaryGoogleAccountUsage,
    AccountAddedForEnterpriseAccountWithPolicyValueAllUsages) {
  base::RunLoop close_dialog_closure_run_loop;

  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  // Set user info response with hosted domain (hd) value.
  AddResponseGetUserInfoWithHostedDomain(kFakeEnterpriseDomain);
  // Set SecondaryGoogleAccountUsage policy fetch to all.
  AddResponseGetSecondaryGoogleAccountUsage("all");
  // Enterprise account tries to sign in.
  CreateSigninHelper(
      /*close_dialog_closure=*/close_dialog_closure_run_loop.QuitClosure(),
      /*show_signin_blocked_by_policy_page=*/
      base::RepeatingClosure(), kFakeEnterpriseGaiaId, kFakeEnterpriseEmail);
  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();

  // 1 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 1);

  auto account = on_token_upserted_account();
  ASSERT_TRUE(account.has_value());
  EXPECT_EQ(account.value().raw_email, kFakeEnterpriseEmail);
  histogram_tester_.ExpectBucketCount(
      kSecondaryGoogleAccountUsageHistogramName,
      ash::UserCloudSigninRestrictionPolicyFetcherChromeOS::Status::kSuccess,
      1);
  histogram_tester_.ExpectTotalCount(
      kSecondaryGoogleAccountUsageLatencyHistogramName, 1);
}

IN_PROC_BROWSER_TEST_F(
    SigninHelperChromeOSTestSecondaryGoogleAccountUsage,
    NoAccountAddedForEnterpriseAccountWithPolicyValuePrimaryAccountSignin) {
  base::RunLoop show_signin_blocked_error_closure_run_loop;

  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  // Set user info response with hosted domain (hd) value.
  AddResponseGetUserInfoWithHostedDomain(kFakeEnterpriseDomain);
  // Set SecondaryGoogleAccountUsage policy fetch to primary_account_signin.
  AddResponseGetSecondaryGoogleAccountUsage("primary_account_signin");
  // Set response for token revokation.
  AddResponseRevokeGaiaTokenOnServer();
  // Enterprise account tries to sign in.
  CreateSigninHelper(
      /*close_dialog_closure=*/base::RepeatingClosure(),
      /*show_signin_blocked_by_policy_page=*/
      show_signin_blocked_error_closure_run_loop.QuitClosure(),
      kFakeEnterpriseGaiaId, kFakeEnterpriseEmail);
  // Make sure the show_signin_blocked_error_closure_run_loop was called.
  show_signin_blocked_error_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();

  // 0 account should be added.
  EXPECT_EQ(on_token_upserted_call_count(), 0);
  histogram_tester_.ExpectBucketCount(
      kSecondaryGoogleAccountUsageHistogramName,
      ash::UserCloudSigninRestrictionPolicyFetcherChromeOS::Status::kSuccess,
      1);
  histogram_tester_.ExpectTotalCount(
      kSecondaryGoogleAccountUsageLatencyHistogramName, 1);
}

IN_PROC_BROWSER_TEST_F(SigninHelperChromeOSTestSecondaryGoogleAccountUsage,
                       ReauthForInitialPrimaryEnterpriseAccount) {
  base::RunLoop close_dialog_closure_run_loop;
  // Set auth token fetch to succeed.
  AddResponseClientOAuthSuccess();
  CreateSigninHelper(
      /*close_dialog_closure=*/close_dialog_closure_run_loop.QuitClosure(),
      /*show_signin_blocked_by_policy_page=*/
      base::RepeatingClosure(),
      user_manager::UserManager::Get()
          ->GetPrimaryUser()
          ->GetAccountId()
          .GetGaiaId(),
      kFakePrimaryEmail);

  // Make sure the close_dialog_closure was called.
  close_dialog_closure_run_loop.Run();
  // Wait until SigninHelper finishes and deletes itself.
  base::RunLoop().RunUntilIdle();

  // 1 account should be upserted.
  EXPECT_EQ(on_token_upserted_call_count(), 1);
  histogram_tester_.ExpectBucketCount(
      kSecondaryGoogleAccountUsageHistogramName,
      ash::UserCloudSigninRestrictionPolicyFetcherChromeOS::Status::
          kUnsupportedAccountTypeError,
      0);
}

}  // namespace chromeos
