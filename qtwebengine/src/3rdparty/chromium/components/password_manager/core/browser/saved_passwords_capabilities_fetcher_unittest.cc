// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/saved_passwords_capabilities_fetcher.h"

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/gmock_move_support.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/values.h"
#include "components/password_manager/core/browser/capabilities_service.h"
#include "components/password_manager/core/browser/fake_password_store_backend.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/site_affiliation/mock_affiliation_service.h"
#include "components/password_manager/core/browser/ui/saved_passwords_presenter.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {
namespace {

using ::base::test::RunOnceCallback;
using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::NiceMock;
using ::testing::Pair;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::StaticAssertTypeEq;
using ::testing::UnorderedElementsAre;
using testing::WithArgs;

constexpr char kOriginWithScript1[] = "https://example.com";
constexpr char kOriginWithScript2[] = "https://mobile.example.com";
constexpr char kOriginWithScript3[] = "https://test.com";
constexpr char kOriginWithScript4[] = "https://test.co.uk";
constexpr char kOriginWithoutScript[] = "https://no-script.com";
constexpr char kExampleApp[] = "android://hash@com.example.app";
constexpr char kHttpOriginWithScript[] = "http://scheme-example.com";

constexpr char16_t kUsername1[] = u"alice";
constexpr char16_t kUsername2[] = u"bob";

constexpr char16_t kPassword1[] = u"f00b4r";
constexpr char16_t kPassword2[] = u"s3cr3t";
constexpr char16_t kPassword3[] = u"skpr2t";
constexpr char16_t kPassword4[] = u"484her";

url::Origin GetOriginWithScript1() {
  return url::Origin::Create(GURL(kOriginWithScript1));
}

url::Origin GetOriginWithScript2() {
  return url::Origin::Create(GURL(kOriginWithScript2));
}

url::Origin GetOriginWithScript3() {
  return url::Origin::Create(GURL(kOriginWithScript3));
}

url::Origin GetOriginWithScript4() {
  return url::Origin::Create(GURL(kOriginWithScript4));
}

url::Origin GetOriginWithoutScript() {
  return url::Origin::Create(GURL(kOriginWithoutScript));
}

PasswordForm MakeSavedPassword(base::StringPiece signon_realm,
                               base::StringPiece16 username,
                               base::StringPiece16 password,
                               base::StringPiece16 username_element = u"") {
  PasswordForm form;
  form.signon_realm = std::string(signon_realm);
  form.url = GURL(signon_realm);
  form.username_value = std::u16string(username);
  form.password_value = std::u16string(password);
  form.username_element = std::u16string(username_element);
  return form;
}

PasswordForm MakeSavedAndroidPassword(
    std::string package_name,
    base::StringPiece16 username,
    base::StringPiece app_display_name = "",
    base::StringPiece affiliated_web_realm = "",
    base::StringPiece16 password = kPassword1) {
  PasswordForm form;
  form.signon_realm = package_name;
  form.username_value = std::u16string(username);
  form.app_display_name = std::string(app_display_name);
  form.affiliated_web_realm = std::string(affiliated_web_realm);
  form.password_value = std::u16string(password);
  return form;
}

class MockCapabilitiesService : public password_manager::CapabilitiesService {
 public:
  MockCapabilitiesService() = default;
  ~MockCapabilitiesService() override = default;

  MOCK_METHOD(void,
              QueryPasswordChangeScriptAvailability,
              (const std::vector<url::Origin>& origins,
               ResponseCallback callback),
              (override));
};

}  // namespace

class SavedPasswordsCapabilitiesFetcherTest : public ::testing::Test {
 public:
  SavedPasswordsCapabilitiesFetcherTest() {
    profile_store_ = base::MakeRefCounted<PasswordStore>(
        std::make_unique<FakePasswordStoreBackend>(IsAccountStore(false)));
    profile_store_->Init(/*prefs=*/nullptr,
                         /*affiliated_match_helper=*/nullptr);

    // The account store is on its own task runner to allow simulating delays.
    account_store_ = base::MakeRefCounted<PasswordStore>(
        std::make_unique<FakePasswordStoreBackend>(
            IsAccountStore(true), account_store_backend_runner()));
    account_store_->Init(/*prefs=*/nullptr,
                         /*affiliated_match_helper=*/nullptr);
    FillProfileStore();
    FillAccountStore();
    CreateFetcher();
  }

  ~SavedPasswordsCapabilitiesFetcherTest() override {
    account_store_->ShutdownOnUIThread();
    profile_store_->ShutdownOnUIThread();
    RunUntilAllThreadsIdle();
  }

  void CreateFetcher(bool use_account_store = false) {
    auto capabilities_service =
        std::make_unique<NiceMock<MockCapabilitiesService>>();
    mock_capabilities_service_ = capabilities_service.get();
    fetcher_ = std::make_unique<SavedPasswordsCapabilitiesFetcher>(
        std::move(capabilities_service),
        std::make_unique<SavedPasswordsPresenter>(
            &affiliation_service_, profile_store_,
            use_account_store ? account_store_ : nullptr));
    RunUntilIdle();
  }

  void FillProfileStore() {
    profile_store_->AddLogin(
        MakeSavedPassword(kOriginWithScript1, kUsername1, kPassword1));
    profile_store_->AddLogin(
        MakeSavedPassword(kOriginWithScript2, kUsername1, kPassword2));
    profile_store_->AddLogin(
        MakeSavedPassword(kOriginWithScript3, kUsername2, kPassword3));
    profile_store_->AddLogin(
        MakeSavedPassword(kOriginWithoutScript, kUsername2, kPassword4));
    profile_store_->AddLogin(
        MakeSavedAndroidPassword(kExampleApp, kUsername2, "Example App",
                                 kOriginWithScript1, kPassword1));
    // Set http url. Should not be made part of the cache.
    profile_store_->AddLogin(
        MakeSavedPassword(kHttpOriginWithScript, kUsername2, kPassword3));

    RunUntilIdle();
  }

  void FillAccountStore() {
    account_store_->AddLogin(
        MakeSavedPassword(kOriginWithScript4, kUsername1, kPassword2));
    RunUntilAllThreadsIdle();
  }

  void RunUntilIdle() { task_env_.RunUntilIdle(); }
  void RunUntilAllThreadsIdle() {
    account_store_backend_runner()->RunUntilIdle();
    RunUntilIdle();
  }

  void CheckScriptAvailabilityDefaultResults() {
    EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
    EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
    EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
    EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));
  }

  void RequestSingleScriptAvailability(const url::Origin& origin) {
    fetcher()->FetchScriptAvailability(
        origin,
        base::BindOnce(&SavedPasswordsCapabilitiesFetcherTest::RecordResponse,
                       base::Unretained(this), origin));
  }

  void RecordResponse(url::Origin origin, bool has_script) {
    const auto& it = recorded_responses_.find(origin);
    if (it != recorded_responses_.end()) {
      DCHECK(recorded_responses_[origin] == has_script)
          << "Responses for " << origin << " differ";
    } else {
      recorded_responses_[origin] = has_script;
    }
  }

  void ExpectCacheRefresh() {
    // Also checks the http credential is not part of the cache.
    EXPECT_CALL(*mock_capabilities_service_,
                QueryPasswordChangeScriptAvailability(
                    UnorderedElementsAre(
                        GetOriginWithScript1(), GetOriginWithScript2(),
                        GetOriginWithScript3(), GetOriginWithoutScript()),
                    _))
        .WillOnce(RunOnceCallback<1>(std::set<url::Origin>{
            GetOriginWithScript1(), GetOriginWithScript2(),
            GetOriginWithScript3()}));
  }

  const scoped_refptr<base::TestMockTimeTaskRunner>&
  account_store_backend_runner() {
    return account_store_backend_runner_;
  }

  SavedPasswordsCapabilitiesFetcher* fetcher() {
    if (!fetcher_)
      CreateFetcher();
    return fetcher_.get();
  }

 protected:
  base::test::SingleThreadTaskEnvironment task_env_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  // Used to simulate delayed responses by the account store.
  scoped_refptr<base::TestMockTimeTaskRunner> account_store_backend_runner_ =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>();

  base::flat_map<url::Origin, bool> recorded_responses_;
  raw_ptr<NiceMock<MockCapabilitiesService>> mock_capabilities_service_;
  scoped_refptr<PasswordStore> profile_store_ = nullptr;
  scoped_refptr<PasswordStore> account_store_ = nullptr;
  std::unique_ptr<SavedPasswordsCapabilitiesFetcher> fetcher_;
  MockAffiliationService affiliation_service_;
};

TEST_F(SavedPasswordsCapabilitiesFetcherTest, ServerError) {
  // Simulate server error empty response.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>()));
  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, PrewarmCache) {
  base::HistogramTester histogram_tester;
  ExpectCacheRefresh();
  EXPECT_TRUE(fetcher()->IsCacheStale());
  fetcher()->PrewarmCache();
  EXPECT_FALSE(fetcher()->IsCacheStale());

  // The cache is not stale yet. No new request is expected.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);

  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());
  EXPECT_FALSE(fetcher()->IsCacheStale());
  CheckScriptAvailabilityDefaultResults();

  // Make cache stale again.
  RunUntilIdle();
  task_env_.AdvanceClock(base::Minutes(10));
  EXPECT_TRUE(fetcher()->IsCacheStale());
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  UnorderedElementsAre(
                      GetOriginWithScript1(), GetOriginWithScript2(),
                      GetOriginWithScript3(), GetOriginWithoutScript()),
                  _))
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>()));
  fetcher()->PrewarmCache();
  EXPECT_FALSE(fetcher()->IsCacheStale());

  histogram_tester.ExpectUniqueSample(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState",
      PasswordScriptsFetcher::CacheState::kReady, 1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      2u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, RefreshScriptsWithTwoStores) {
  // Explicitly force the fetcher creation to ensure it has an account store.
  CreateFetcher(/*use_account_store=*/true);
  EXPECT_TRUE(fetcher()->IsCacheStale());

  base::MockCallback<base::OnceClosure> callback;
  fetcher()->RefreshScriptsIfNecessary(callback.Get());
  // The cache remains stale while the account store has not answered.
  RunUntilIdle();
  EXPECT_TRUE(fetcher()->IsCacheStale());

  EXPECT_CALL(
      *mock_capabilities_service_,
      QueryPasswordChangeScriptAvailability(
          UnorderedElementsAre(GetOriginWithScript1(), GetOriginWithScript2(),
                               GetOriginWithScript3(), GetOriginWithoutScript(),
                               GetOriginWithScript4()),
          _))
      .WillOnce(RunOnceCallback<1>(
          std::set<url::Origin>{GetOriginWithScript1(), GetOriginWithScript2(),
                                GetOriginWithScript3()}));
  EXPECT_CALL(callback, Run);
  RunUntilAllThreadsIdle();
  EXPECT_FALSE(fetcher()->IsCacheStale());

  // The cache is not stale anymore. No new request is expected.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, NoPrewarmCache) {
  base::HistogramTester histogram_tester;
  // Run bulk check with no cache prewarming. Expect necessary full refresh.
  ExpectCacheRefresh();
  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());
  CheckScriptAvailabilityDefaultResults();

  histogram_tester.ExpectUniqueSample(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState",
      PasswordScriptsFetcher::CacheState::kStale, 1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest,
       StartBulkCheckBeforePrewarmingResponse) {
  base::HistogramTester histogram_tester;

  CapabilitiesService::ResponseCallback callback;
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  UnorderedElementsAre(
                      GetOriginWithScript1(), GetOriginWithScript2(),
                      GetOriginWithScript3(), GetOriginWithoutScript()),
                  _))
      .WillOnce(MoveArg<1>(&callback));

  fetcher()->PrewarmCache();

  // Bulk check started before server's prewarming response. No new request
  // should be triggered if the cache is |kWaiting|.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());

  // Resolve prewarming callback.
  std::move(callback).Run(std::set<url::Origin>{
      GetOriginWithScript1(), GetOriginWithScript2(), GetOriginWithScript3()});
  CheckScriptAvailabilityDefaultResults();
  histogram_tester.ExpectUniqueSample(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState",
      PasswordScriptsFetcher::CacheState::kWaiting, 1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, IsScriptAvailable) {
  base::HistogramTester histogram_tester;
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
  // |IsScriptAvailable| does not trigger any network requests and returns the
  // default value (false).
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));

  ExpectCacheRefresh();
  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());

  // Cache is ready.
  CheckScriptAvailabilityDefaultResults();

  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);

  // Make cache stale again.
  task_env_.AdvanceClock(base::Minutes(10));
  // |IsScriptAvailable| does not trigger refetching and returns false.
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));

  histogram_tester.ExpectUniqueSample(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState",
      PasswordScriptsFetcher::CacheState::kStale, 1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest,
       EnablePasswordDomainCapabilitiesFlag) {
  // |kEnablePasswordDomainCapabilities| flag is disabled, |IsScriptAvailable|
  // returns the default value (false).
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_FALSE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));

  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      password_manager::features::kForceEnablePasswordDomainCapabilities);

  // |kEnablePasswordDomainCapabilities| is enabled and all scripts should have
  // capabilities enabled.
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));

  ExpectCacheRefresh();
  fetcher()->RefreshScriptsIfNecessary(base::DoNothing());

  // Cache is ready.
  // All scripts should have capabilities regardless of the server response.
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));

  // Make cache stale again.
  task_env_.AdvanceClock(base::Minutes(10));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript1()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript2()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithScript3()));
  EXPECT_TRUE(fetcher()->IsScriptAvailable(GetOriginWithoutScript()));
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, PasswordStoreUpdate) {
  ExpectCacheRefresh();
  fetcher()->PrewarmCache();

  // Add a new login to the store. Cache should go stale.
  PasswordForm password_form =
      MakeSavedPassword("https://foo.com", kUsername1, kPassword1);
  profile_store_->AddLogin(password_form);
  RunUntilIdle();

  // Expect refresh of stored creentials including the new one.
  EXPECT_CALL(
      *mock_capabilities_service_,
      QueryPasswordChangeScriptAvailability(
          UnorderedElementsAre(GetOriginWithScript1(), GetOriginWithScript2(),
                               GetOriginWithScript3(), GetOriginWithoutScript(),
                               url::Origin::Create(GURL("https://foo.com"))),
          _))
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>()));
  fetcher()->PrewarmCache();

  // Updated a credential, cache should *not* go stale.
  password_form.password_value = kPassword2;
  profile_store_->UpdateLogin(password_form);
  RunUntilIdle();

  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
  fetcher()->PrewarmCache();
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest,
       FetchScriptAvailabilityDuringRequest) {
  base::HistogramTester histogram_tester;

  CapabilitiesService::ResponseCallback callback;
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  UnorderedElementsAre(
                      GetOriginWithScript1(), GetOriginWithScript2(),
                      GetOriginWithScript3(), GetOriginWithoutScript()),
                  _))
      .WillOnce(MoveArg<1>(&callback));

  fetcher()->PrewarmCache();

  // FetchScriptAvailability before server's prewarming response. No new request
  // should be triggered if the cache is |kWaiting|. Requests should be answered
  // after refresh is finished.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
  RequestSingleScriptAvailability(GetOriginWithScript1());
  RequestSingleScriptAvailability(GetOriginWithoutScript());

  // Resolve prewarming callback.
  std::move(callback).Run(std::set<url::Origin>{
      GetOriginWithScript1(), GetOriginWithScript2(), GetOriginWithScript3()});
  EXPECT_THAT(recorded_responses_,
              UnorderedElementsAre(Pair(GetOriginWithScript1(), true),
                                   Pair(GetOriginWithoutScript(), false)));
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState", 0u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "SingleOriginResponseTime",
      0u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest,
       FetchScriptAvailabilityAfterRefreshRequest) {
  base::HistogramTester histogram_tester;

  ExpectCacheRefresh();
  fetcher()->PrewarmCache();

  // Add a new login to the store. Cache should go stale and
  // FetchScriptAvailability should trigger single origin requests.
  PasswordForm password_form =
      MakeSavedPassword("https://foo.com", kUsername1, kPassword1);
  profile_store_->AddLogin(password_form);
  RunUntilIdle();

  url::Origin foo_origin = url::Origin::Create(GURL("https://foo.com"));
  EXPECT_CALL(
      *mock_capabilities_service_,
      QueryPasswordChangeScriptAvailability(ElementsAreArray({foo_origin}), _))
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>{foo_origin}));

  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  ElementsAreArray({GetOriginWithoutScript()}), _))
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>()));

  // Origin got added to the cache, record should be stale and therefore
  // trigger a single origin request.
  RequestSingleScriptAvailability(foo_origin);
  RequestSingleScriptAvailability(GetOriginWithoutScript());

  EXPECT_THAT(recorded_responses_,
              UnorderedElementsAre(Pair(foo_origin, true),
                                   Pair(GetOriginWithoutScript(), false)));

  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState", 0u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "SingleOriginResponseTime",
      2u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest,
       FetchScriptAvailabilityAfterStaleCache) {
  base::HistogramTester histogram_tester;

  // FetchScriptAvailability without any refresh should trigger single origin
  // request.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  ElementsAreArray({GetOriginWithScript1()}), _))
      .WillOnce(
          RunOnceCallback<1>(std::set<url::Origin>{GetOriginWithScript1()}));

  RequestSingleScriptAvailability(GetOriginWithScript1());
  EXPECT_THAT(recorded_responses_,
              ElementsAreArray({Pair(GetOriginWithScript1(), true)}));

  // Refresh full cache.
  ExpectCacheRefresh();
  fetcher()->PrewarmCache();

  // The cache is not stale. No new request is expected.
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability)
      .Times(0);
  RequestSingleScriptAvailability(GetOriginWithScript1());
  EXPECT_THAT(recorded_responses_,
              ElementsAreArray({Pair(GetOriginWithScript1(), true)}));

  // Cache goes stale. Single origin request is expected.
  task_env_.AdvanceClock(base::Minutes(10));
  recorded_responses_.clear();

  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  ElementsAreArray({GetOriginWithScript2()}), _))
      .WillOnce(
          RunOnceCallback<1>(std::set<url::Origin>{GetOriginWithScript2()}));

  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  ElementsAreArray({GetOriginWithoutScript()}), _))
      .WillOnce(RunOnceCallback<1>(std::set<url::Origin>()));

  RequestSingleScriptAvailability(GetOriginWithScript2());
  RequestSingleScriptAvailability(GetOriginWithoutScript());

  EXPECT_THAT(recorded_responses_,
              UnorderedElementsAre(Pair(GetOriginWithScript2(), true),
                                   Pair(GetOriginWithoutScript(), false)));
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher.CacheState", 0u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "AllOriginsResponseTime",
      1u);
  histogram_tester.ExpectTotalCount(
      "PasswordManager.SavedPasswordsCapabilitiesFetcher."
      "SingleOriginResponseTime",
      3u);
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, DebugInformationForInternals) {
  base::Value::Dict debug_info = fetcher()->GetDebugInformationForInternals();
  const std::string* engine = debug_info.FindString("engine");
  EXPECT_TRUE(engine);
  EXPECT_EQ("hash-prefix-based lookup", *engine);

  const std::string* cache_state = debug_info.FindString("cache state");
  EXPECT_TRUE(cache_state);
  // Cache is already stale instead of never set due to a call during SetUp().
  EXPECT_EQ("stale", *cache_state);

  ExpectCacheRefresh();
  fetcher()->PrewarmCache();

  debug_info = fetcher()->GetDebugInformationForInternals();
  cache_state = debug_info.FindString("cache state");
  EXPECT_TRUE(cache_state);
  EXPECT_EQ("ready", *cache_state);

  // Make cache stale again.
  RunUntilIdle();
  task_env_.AdvanceClock(base::Minutes(10));

  debug_info = fetcher()->GetDebugInformationForInternals();
  cache_state = debug_info.FindString("cache state");
  EXPECT_TRUE(cache_state);
  EXPECT_EQ("stale", *cache_state);

  // Create a state in which the fetcher is waiting for a response.
  CapabilitiesService::ResponseCallback callback;
  EXPECT_CALL(*mock_capabilities_service_,
              QueryPasswordChangeScriptAvailability(
                  UnorderedElementsAre(
                      GetOriginWithScript1(), GetOriginWithScript2(),
                      GetOriginWithScript3(), GetOriginWithoutScript()),
                  _))
      .WillOnce(MoveArg<1>(&callback));

  fetcher()->PrewarmCache();
  debug_info = fetcher()->GetDebugInformationForInternals();
  cache_state = debug_info.FindString("cache state");
  EXPECT_TRUE(cache_state);
  EXPECT_EQ("waiting", *cache_state);

  std::move(callback).Run(std::set<url::Origin>{
      GetOriginWithScript1(), GetOriginWithScript2(), GetOriginWithScript3()});
  CheckScriptAvailabilityDefaultResults();
}

TEST_F(SavedPasswordsCapabilitiesFetcherTest, CheckCacheEntries) {
  ExpectCacheRefresh();
  fetcher()->PrewarmCache();

  // Cache should now contain four entries.
  base::Value::List cache_entries = fetcher()->GetCacheEntries();
  EXPECT_EQ(cache_entries.size(), 4u);

  std::vector<std::string> urls;
  // Only `kOriginWithoutScript` is not expected to have a script.
  for (const auto& element : cache_entries) {
    const base::Value::Dict& entry = element.GetDict();
    const std::string* url = entry.FindString("url");
    absl::optional<bool> has_script = entry.FindBool("has_script");
    EXPECT_TRUE(url);
    EXPECT_TRUE(has_script.has_value());
    EXPECT_EQ(*url != kOriginWithoutScript, has_script.value());
    urls.push_back(*url);
  }

  // There should be entries for all requested sites.
  EXPECT_THAT(urls,
              UnorderedElementsAre(kOriginWithoutScript, kOriginWithScript1,
                                   kOriginWithScript2, kOriginWithScript3));

  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      password_manager::features::kForceEnablePasswordDomainCapabilities);
  // Now all domains should return available scripts.
  cache_entries = fetcher()->GetCacheEntries();
  EXPECT_EQ(cache_entries.size(), 4u);
  EXPECT_TRUE(base::ranges::all_of(
      cache_entries.cbegin(), cache_entries.cend(),
      [](const base::Value& element) {
        return element.GetDict().FindBool("has_script").value_or(false);
      }));
}

}  // namespace password_manager
