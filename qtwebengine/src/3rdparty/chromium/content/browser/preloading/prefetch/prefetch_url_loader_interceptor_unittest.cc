// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/preloading/prefetch/prefetch_url_loader_interceptor.h"

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/preloading/prefetch/prefetch_container.h"
#include "content/browser/preloading/prefetch/prefetch_features.h"
#include "content/browser/preloading/prefetch/prefetch_origin_prober.h"
#include "content/browser/preloading/prefetch/prefetch_params.h"
#include "content/browser/preloading/prefetch/prefetch_probe_result.h"
#include "content/browser/preloading/prefetch/prefetch_type.h"
#include "content/browser/preloading/prefetch/prefetched_mainframe_response_container.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/navigation_request.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/test/test_content_browser_client.h"
#include "net/base/isolation_info.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace content {
namespace {

// These tests leak mojo objects (like the PrefetchFromStringURLLoader) because
// they do not have valid mojo channels, which would normally delete the bound
// objects on destruction. This is expected and cannot be easily fixed without
// rewriting these as browsertests. The trade off for the speed and flexibility
// of unittests is an intentional decision.
#if defined(LEAK_SANITIZER)
#define DISABLE_ASAN(x) DISABLED_##x
#else
#define DISABLE_ASAN(x) x
#endif

const char kDNSCanaryCheckAddress[] = "http://testdnscanarycheck.com";
const char kTLSCanaryCheckAddress[] = "http://testtlscanarycheck.com";

class TestPrefetchOriginProber : public PrefetchOriginProber {
 public:
  TestPrefetchOriginProber(BrowserContext* browser_context,
                           bool should_probe_origins_response,
                           const GURL& probe_url,
                           PrefetchProbeResult probe_result)
      : PrefetchOriginProber(browser_context,
                             GURL(kDNSCanaryCheckAddress),
                             GURL(kTLSCanaryCheckAddress)),
        should_probe_origins_response_(should_probe_origins_response),
        probe_url_(probe_url),
        probe_result_(probe_result) {}

  bool ShouldProbeOrigins() const override {
    return should_probe_origins_response_;
  }

  void Probe(const GURL& url, OnProbeResultCallback callback) override {
    EXPECT_TRUE(should_probe_origins_response_);
    EXPECT_EQ(url, probe_url_);

    num_probes_++;

    std::move(callback).Run(probe_result_);
  }

  int num_probes() const { return num_probes_; }

 private:
  bool should_probe_origins_response_;

  GURL probe_url_;
  PrefetchProbeResult probe_result_;

  int num_probes_{0};
};

class ScopedMockContentBrowserClient : public TestContentBrowserClient {
 public:
  ScopedMockContentBrowserClient() {
    old_browser_client_ = SetBrowserClientForTesting(this);
  }

  ~ScopedMockContentBrowserClient() override {
    EXPECT_EQ(this, SetBrowserClientForTesting(old_browser_client_));
  }

  MOCK_METHOD(
      bool,
      WillCreateURLLoaderFactory,
      (BrowserContext * browser_context,
       RenderFrameHost* frame,
       int render_process_id,
       URLLoaderFactoryType type,
       const url::Origin& request_initiator,
       absl::optional<int64_t> navigation_id,
       ukm::SourceIdObj ukm_source_id,
       mojo::PendingReceiver<network::mojom::URLLoaderFactory>*
           factory_receiver,
       mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
           header_client,
       bool* bypass_redirect_checks,
       bool* disable_secure_dns,
       network::mojom::URLLoaderFactoryOverridePtr* factory_override),
      (override));

 private:
  raw_ptr<ContentBrowserClient> old_browser_client_;
};

class TestPrefetchURLLoaderInterceptor : public PrefetchURLLoaderInterceptor {
 public:
  explicit TestPrefetchURLLoaderInterceptor(int frame_tree_node_id)
      : PrefetchURLLoaderInterceptor(frame_tree_node_id) {}
  ~TestPrefetchURLLoaderInterceptor() override = default;

  void AddPrefetch(base::WeakPtr<PrefetchContainer> prefetch_container) {
    prefetches_[prefetch_container->GetURL()] = prefetch_container;
  }

  void TakePrefetchOriginProber(
      std::unique_ptr<TestPrefetchOriginProber> origin_prober) {
    origin_prober_ = std::move(origin_prober);
  }

  int num_probes() const { return origin_prober_->num_probes(); }

 private:
  base::WeakPtr<PrefetchContainer> GetPrefetch(const GURL& url) const override {
    const auto& iter = prefetches_.find(url);
    if (iter == prefetches_.end())
      return nullptr;
    return iter->second;
  }

  PrefetchOriginProber* GetPrefetchOriginProber() const override {
    EXPECT_TRUE(origin_prober_);
    return origin_prober_.get();
  }

  std::map<GURL, base::WeakPtr<PrefetchContainer>> prefetches_;
  std::unique_ptr<TestPrefetchOriginProber> origin_prober_;
};

class PrefetchURLLoaderInterceptorTest : public RenderViewHostTestHarness {
 public:
  PrefetchURLLoaderInterceptorTest()
      : RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    RenderViewHostTestHarness::SetUp();

    test_content_browser_client_ =
        std::make_unique<ScopedMockContentBrowserClient>();

    browser_context()
        ->GetDefaultStoragePartition()
        ->GetNetworkContext()
        ->GetCookieManager(cookie_manager_.BindNewPipeAndPassReceiver());

    auto navigation_simulator = NavigationSimulator::CreateBrowserInitiated(
        GURL("https://test.com"), web_contents());
    navigation_simulator->Start();

    interceptor_ = std::make_unique<TestPrefetchURLLoaderInterceptor>(
        web_contents()->GetPrimaryMainFrame()->GetFrameTreeNodeId());
  }

  void TearDown() override {
    interceptor_.release();

    RenderViewHostTestHarness::TearDown();
  }

  TestPrefetchURLLoaderInterceptor* interceptor() { return interceptor_.get(); }

  void WaitForCallback() {
    if (was_intercepted_.has_value())
      return;

    base::RunLoop run_loop;
    on_loader_callback_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void LoaderCallback(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
    was_intercepted_ = url_loader_factory != nullptr;
    if (on_loader_callback_closure_) {
      std::move(on_loader_callback_closure_).Run();
    }
  }

  absl::optional<bool> was_intercepted() { return was_intercepted_; }

  NavigationRequest* navigation_request() {
    return FrameTreeNode::GloballyFindByID(
               web_contents()->GetPrimaryMainFrame()->GetFrameTreeNodeId())
        ->navigation_request();
  }

  bool SetCookie(const GURL& url, const std::string& value) {
    bool result = false;
    base::RunLoop run_loop;

    std::unique_ptr<net::CanonicalCookie> cookie(net::CanonicalCookie::Create(
        url, value, base::Time::Now(), /*server_time=*/absl::nullopt,
        /*cookie_partition_key=*/absl::nullopt));
    EXPECT_TRUE(cookie.get());
    EXPECT_TRUE(cookie->IsHostCookie());

    net::CookieOptions options;
    options.set_include_httponly();
    options.set_same_site_cookie_context(
        net::CookieOptions::SameSiteCookieContext::MakeInclusive());

    cookie_manager_->SetCanonicalCookie(
        *cookie.get(), url, options,
        base::BindOnce(
            [](bool* result, base::RunLoop* run_loop,
               net::CookieAccessResult set_cookie_access_result) {
              *result = set_cookie_access_result.status.IsInclude();
              run_loop->Quit();
            },
            &result, &run_loop));

    // This will run until the cookie is set.
    run_loop.Run();

    // This will run until the cookie listener gets the cookie change.
    base::RunLoop().RunUntilIdle();

    return result;
  }

  network::mojom::CookieManager* cookie_manager() {
    return cookie_manager_.get();
  }

  const base::HistogramTester& histogram_tester() { return histogram_tester_; }

  ScopedMockContentBrowserClient* test_content_browser_client() {
    return test_content_browser_client_.get();
  }

 private:
  std::unique_ptr<TestPrefetchURLLoaderInterceptor> interceptor_;

  base::HistogramTester histogram_tester_;

  absl::optional<bool> was_intercepted_;
  base::OnceClosure on_loader_callback_closure_;

  mojo::Remote<network::mojom::CookieManager> cookie_manager_;
  std::unique_ptr<ScopedMockContentBrowserClient> test_content_browser_client_;
};

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(InterceptNavigationCookieCopyCompleted)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(
      *test_content_browser_client(),
      WillCreateURLLoaderFactory(
          testing::NotNull(), main_rfh(), main_rfh()->GetProcess()->GetID(),
          ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          testing::ResultOf(
              [](const url::Origin& request_initiator) {
                return request_initiator.opaque();
              },
              true),
          testing::Optional(navigation_request()->GetNavigationId()),
          ukm::SourceIdObj::FromInt64(
              navigation_request()->GetNextPageUkmSourceId()),
          testing::NotNull(), testing::IsNull(), testing::NotNull(),
          testing::IsNull(), testing::IsNull()))
      .WillOnce(testing::Return(false));

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  // Simulate the cookie copy process starting and finishing before
  // |MaybeCreateLoader| is called.
  prefetch_container->OnIsolatedCookieCopyStart();
  task_environment()->FastForwardBy(base::Milliseconds(10));
  prefetch_container->OnIsolatedCookieCopyComplete();

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_TRUE(was_intercepted().value());

  histogram_tester().ExpectUniqueTimeSample(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", base::TimeDelta(),
      1);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(InterceptNavigationCookieCopyInProgress)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(
      *test_content_browser_client(),
      WillCreateURLLoaderFactory(
          testing::NotNull(), main_rfh(), main_rfh()->GetProcess()->GetID(),
          ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          testing::ResultOf(
              [](const url::Origin& request_initiator) {
                return request_initiator.opaque();
              },
              true),
          testing::Optional(navigation_request()->GetNavigationId()),
          ukm::SourceIdObj::FromInt64(
              navigation_request()->GetNextPageUkmSourceId()),
          testing::NotNull(), testing::IsNull(), testing::NotNull(),
          testing::IsNull(), testing::IsNull()))
      .WillOnce(testing::Return(false));

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  // Simulate the cookie copy process starting, but not finishing until after
  // |MaybeCreateLoader| is called.
  prefetch_container->OnIsolatedCookieCopyStart();
  task_environment()->FastForwardBy(base::Milliseconds(10));

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));

  // A decision on whether the navigation should be intercepted shouldn't be
  // made until after the cookie copy process is completed.
  EXPECT_FALSE(was_intercepted().has_value());

  task_environment()->FastForwardBy(base::Milliseconds(20));

  prefetch_container->OnIsolatedCookieCopyComplete();
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_TRUE(was_intercepted().value());

  histogram_tester().ExpectUniqueTimeSample(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime",
      base::Milliseconds(20), 1);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(InterceptNavigationNoCookieCopyNeeded)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(
      *test_content_browser_client(),
      WillCreateURLLoaderFactory(
          testing::NotNull(), main_rfh(), main_rfh()->GetProcess()->GetID(),
          ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          testing::ResultOf(
              [](const url::Origin& request_initiator) {
                return request_initiator.opaque();
              },
              true),
          testing::Optional(navigation_request()->GetNavigationId()),
          ukm::SourceIdObj::FromInt64(
              navigation_request()->GetNextPageUkmSourceId()),
          testing::NotNull(), testing::IsNull(), testing::NotNull(),
          testing::IsNull(), testing::IsNull()))
      .WillOnce(testing::Return(false));

  // No cookies are copied for prefetches where |use_isolated_network_context|
  // is false (i.e. same origin prefetches).
  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/false,
                       /*use_prefetch_proxy=*/false),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_TRUE(was_intercepted().value());

  histogram_tester().ExpectUniqueTimeSample(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", base::TimeDelta(),
      1);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(DoNotInterceptNavigationNoPrefetch)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(*test_content_browser_client(), WillCreateURLLoaderFactory)
      .Times(0);

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  // With no prefetch set, the navigation shouldn't be intercepted.

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_FALSE(was_intercepted().value());

  histogram_tester().ExpectTotalCount(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", 0);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(DoNotInterceptNavigationNoPrefetchedResponse)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(*test_content_browser_client(), WillCreateURLLoaderFactory)
      .Times(0);

  // Without a prefetched response, the navigation shouldn't be intercepted.
  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  // Set up ResourceRequest
  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  // Try to create loader
  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_FALSE(was_intercepted().value());

  histogram_tester().ExpectTotalCount(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", 0);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(DoNotInterceptNavigationStalePrefetchedResponse)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(*test_content_browser_client(), WillCreateURLLoaderFactory)
      .Times(0);

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  // Advance time enough so that the response is considered stale.
  task_environment()->FastForwardBy(2 * PrefetchCacheableDuration());

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_FALSE(was_intercepted().value());

  histogram_tester().ExpectTotalCount(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", 0);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest,
       DISABLE_ASAN(DoNotInterceptNavigationCookiesChanged)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(*test_content_browser_client(), WillCreateURLLoaderFactory)
      .Times(0);

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  // Since the cookies associated with |kTestUrl| have changed, the prefetch can
  // no longer be served.
  prefetch_container->RegisterCookieListener(cookie_manager());
  ASSERT_TRUE(SetCookie(kTestUrl, "test-cookie"));

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/false, kTestUrl,
          PrefetchProbeResult::kNoProbing));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_FALSE(was_intercepted().value());

  histogram_tester().ExpectTotalCount(
      "PrefetchProxy.AfterClick.Mainframe.CookieWaitTime", 0);

  EXPECT_EQ(interceptor()->num_probes(), 0);
}

TEST_F(PrefetchURLLoaderInterceptorTest, DISABLE_ASAN(ProbeSuccess)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(
      *test_content_browser_client(),
      WillCreateURLLoaderFactory(
          testing::NotNull(), main_rfh(), main_rfh()->GetProcess()->GetID(),
          ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          testing::ResultOf(
              [](const url::Origin& request_initiator) {
                return request_initiator.opaque();
              },
              true),
          testing::Optional(navigation_request()->GetNavigationId()),
          ukm::SourceIdObj::FromInt64(
              navigation_request()->GetNextPageUkmSourceId()),
          testing::NotNull(), testing::IsNull(), testing::NotNull(),
          testing::IsNull(), testing::IsNull()))
      .WillOnce(testing::Return(false));

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  prefetch_container->OnIsolatedCookieCopyStart();
  prefetch_container->OnIsolatedCookieCopyComplete();

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  // Set up |TestPrefetchOriginProber| to require a probe and simulate a
  // successful probe.
  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/true, kTestUrl,
          PrefetchProbeResult::kDNSProbeSuccess));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_TRUE(was_intercepted().value());

  EXPECT_EQ(interceptor()->num_probes(), 1);
}

TEST_F(PrefetchURLLoaderInterceptorTest, DISABLE_ASAN(ProbeFailure)) {
  const GURL kTestUrl("https://example.com");

  EXPECT_CALL(*test_content_browser_client(), WillCreateURLLoaderFactory)
      .Times(0);

  std::unique_ptr<PrefetchContainer> prefetch_container =
      std::make_unique<PrefetchContainer>(
          main_rfh()->GetGlobalId(), kTestUrl,
          PrefetchType(/*use_isolated_network_context=*/true,
                       /*use_prefetch_proxy=*/true),
          blink::mojom::Referrer(), nullptr);

  prefetch_container->TakePrefetchedResponse(
      std::make_unique<PrefetchedMainframeResponseContainer>(
          net::IsolationInfo(), network::mojom::URLResponseHead::New(),
          std::make_unique<std::string>("test body")));

  prefetch_container->OnIsolatedCookieCopyStart();
  prefetch_container->OnIsolatedCookieCopyComplete();

  interceptor()->AddPrefetch(prefetch_container->GetWeakPtr());

  // Set up |TestPrefetchOriginProber| to require a probe and simulate a
  // successful probe.
  interceptor()->TakePrefetchOriginProber(
      std::make_unique<TestPrefetchOriginProber>(
          browser_context(), /*should_probe_origins_response=*/true, kTestUrl,
          PrefetchProbeResult::kDNSProbeFailure));

  network::ResourceRequest request;
  request.url = kTestUrl;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.method = "GET";

  interceptor()->MaybeCreateLoader(
      request, browser_context(),
      base::BindOnce(&PrefetchURLLoaderInterceptorTest::LoaderCallback,
                     base::Unretained(this)),
      base::BindOnce([](bool) { NOTREACHED(); }));
  WaitForCallback();

  EXPECT_TRUE(was_intercepted().has_value());
  EXPECT_FALSE(was_intercepted().value());

  EXPECT_EQ(interceptor()->num_probes(), 1);
}

}  // namespace
}  // namespace content
