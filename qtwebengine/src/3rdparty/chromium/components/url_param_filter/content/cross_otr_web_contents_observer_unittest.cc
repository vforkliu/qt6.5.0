// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_param_filter/content/cross_otr_web_contents_observer.h"

#include "base/test/metrics/histogram_tester.h"
#include "components/url_param_filter/core/url_param_classifications_loader.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "net/http/http_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"

using ::testing::NiceMock;

namespace url_param_filter {

constexpr char kExperimentalResponseCodeMetricName[] =
    "Navigation.CrossOtr.ContextMenu.ResponseCodeExperimental";
constexpr char kResponseCodeMetricName[] =
    "Navigation.CrossOtr.ContextMenu.ResponseCode";
constexpr char kExperimentalCrossOtrRefreshCountMetricName[] =
    "Navigation.CrossOtr.ContextMenu.RefreshCountExperimental";
constexpr char kCrossOtrRefreshCountMetricName[] =
    "Navigation.CrossOtr.ContextMenu.RefreshCount";

class CrossOtrWebContentsObserverTest
    : public content::RenderViewHostTestHarness {
 public:
  CrossOtrWebContentsObserverTest() = default;
};

TEST_F(CrossOtrWebContentsObserverTest, NotContextMenuInitiated) {
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), /*is_cross_otr=*/false,
      /*started_from_context_menu=*/false, ui::PAGE_TRANSITION_LINK);

  ASSERT_EQ(CrossOtrWebContentsObserver::FromWebContents(web_contents()),
            nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, DefaultSensitivity) {
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), /*is_cross_otr=*/false,
      /*started_from_context_menu=*/false, ui::PAGE_TRANSITION_LINK);

  ASSERT_EQ(CrossOtrWebContentsObserver::FromWebContents(web_contents()),
            nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, BookmarkLink) {
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_AUTO_BOOKMARK);

  ASSERT_EQ(CrossOtrWebContentsObserver::FromWebContents(web_contents()),
            nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, CreateKey) {
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);

  ASSERT_NE(CrossOtrWebContentsObserver::FromWebContents(contents), nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, DuplicateCreateKey) {
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);

  ASSERT_NE(CrossOtrWebContentsObserver::FromWebContents(contents), nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, HandleRedirects) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);

  std::vector<const std::string> redirect_sequence{
      "HTTP/1.1 302 Moved Temporarily",
      "HTTP/1.1 307 Temporary Redirect",  // 2 'external' 307 redirects
      "HTTP/1.1 307 Temporary Redirect",
      "HTTP/1.1 307 Internal Redirect"  // 1 internal 307 redirect, is  omitted
  };
  for (auto redirect_header : redirect_sequence) {
    std::unique_ptr<content::MockNavigationHandle> handle =
        std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

    scoped_refptr<net::HttpResponseHeaders> response =
        base::MakeRefCounted<net::HttpResponseHeaders>(redirect_header);
    handle->set_response_headers(response);
    observer->DidRedirectNavigation(handle.get());
  }
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 3);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(302),
      1);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(307),
      2);
}
TEST_F(CrossOtrWebContentsObserverTest, HandleRedirectsExperimentalMetrics) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(true,
                               ClassificationExperimentStatus::EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);

  std::vector<const std::string> redirect_sequence{
      "HTTP/1.1 302 Moved Temporarily",
      "HTTP/1.1 307 Temporary Redirect",  // 2 'external' 307 redirects
      "HTTP/1.1 307 Temporary Redirect",
      "HTTP/1.1 307 Internal Redirect"  // 1 internal 307 redirect, is  omitted
  };
  for (auto redirect_header : redirect_sequence) {
    std::unique_ptr<content::MockNavigationHandle> handle =
        std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

    scoped_refptr<net::HttpResponseHeaders> response =
        base::MakeRefCounted<net::HttpResponseHeaders>(redirect_header);
    handle->set_response_headers(response);
    observer->DidRedirectNavigation(handle.get());
  }
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 3);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(302),
      1);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(307),
      2);
  histogram_tester.ExpectTotalCount(kExperimentalResponseCodeMetricName, 3);
  histogram_tester.ExpectBucketCount(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(302), 1);
  histogram_tester.ExpectBucketCount(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(307), 2);
}
TEST_F(CrossOtrWebContentsObserverTest,
       HandleRedirectsExperimentalAndNonExperimentalMetrics) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  // The `ClassificationExperimentStatus::EXPERIMENTAL` should take precedence,
  // since we consider an experimental metric filtered anywhere in the
  // navigation to mean the results after are contingent on that parameter's
  // removal.
  observer->SetDidFilterParams(true,
                               ClassificationExperimentStatus::EXPERIMENTAL);
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);

  std::vector<const std::string> redirect_sequence{
      "HTTP/1.1 302 Moved Temporarily",
      "HTTP/1.1 307 Temporary Redirect",  // 2 'external' 307 redirects
      "HTTP/1.1 307 Temporary Redirect",
      "HTTP/1.1 307 Internal Redirect"  // 1 internal 307 redirect, is  omitted
  };
  for (auto redirect_header : redirect_sequence) {
    std::unique_ptr<content::MockNavigationHandle> handle =
        std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

    scoped_refptr<net::HttpResponseHeaders> response =
        base::MakeRefCounted<net::HttpResponseHeaders>(redirect_header);
    handle->set_response_headers(response);
    observer->DidRedirectNavigation(handle.get());
  }
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 3);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(302),
      1);
  histogram_tester.ExpectBucketCount(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(307),
      2);
  histogram_tester.ExpectTotalCount(kExperimentalResponseCodeMetricName, 3);
  histogram_tester.ExpectBucketCount(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(302), 1);
  histogram_tester.ExpectBucketCount(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(307), 2);
}
TEST_F(CrossOtrWebContentsObserverTest, HandleRedirectsNoParamsFiltering) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>(
          "HTTP/1.1 302 Moved Temporarily");
  handle->set_response_headers(response);
  observer->DidRedirectNavigation(handle.get());
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(302),
      0);
}
TEST_F(CrossOtrWebContentsObserverTest, FinishedNavigation) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  ASSERT_TRUE(observer->IsCrossOtrState());
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
}
TEST_F(CrossOtrWebContentsObserverTest,
       FinishedNavigationNonExperimentalThenExperimental) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  observer->SetDidFilterParams(true,
                               ClassificationExperimentStatus::EXPERIMENTAL);
  observer->DidFinishNavigation(handle.get());

  ASSERT_TRUE(observer->IsCrossOtrState());
  // We first filtered params with non-experimental params, then filtered an
  // experimental param. Both metrics should be written, with responses showing
  // in the normal metric and only the experimental result in the experimental
  // metric.
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 2);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      2);
  histogram_tester.ExpectTotalCount(kExperimentalResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(200), 1);
}
TEST_F(CrossOtrWebContentsObserverTest, FinishedNavigationNoParamsFiltering) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  ASSERT_TRUE(observer->IsCrossOtrState());
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      0);
}
TEST_F(CrossOtrWebContentsObserverTest, BadRedirectResponse) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  handle->set_response_headers(nullptr);
  observer->DidRedirectNavigation(handle.get());
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
}
TEST_F(CrossOtrWebContentsObserverTest, BadNavigationResponse) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  handle->set_response_headers(nullptr);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);

  // The observer should not cease observation after first load, regardless of
  // whether the headers include a response code. We still want to see
  // the refresh count.
  ASSERT_NE(CrossOtrWebContentsObserver::FromWebContents(contents), nullptr);
}
TEST_F(CrossOtrWebContentsObserverTest, RefreshedAfterNavigation) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  observer->WebContentsDestroyed();

  histogram_tester.ExpectTotalCount(kCrossOtrRefreshCountMetricName, 1);
  ASSERT_EQ(histogram_tester.GetTotalSum(kCrossOtrRefreshCountMetricName), 1);
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
}
TEST_F(CrossOtrWebContentsObserverTest, RefreshedAfterNavigationExperimental) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(true,
                               ClassificationExperimentStatus::EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  observer->WebContentsDestroyed();

  histogram_tester.ExpectTotalCount(kExperimentalCrossOtrRefreshCountMetricName,
                                    1);
  ASSERT_EQ(
      histogram_tester.GetTotalSum(kExperimentalCrossOtrRefreshCountMetricName),
      1);
  // An experimental classification was used, so both metrics should be written.
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
  histogram_tester.ExpectTotalCount(kExperimentalResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kExperimentalResponseCodeMetricName,
      net::HttpUtil::MapStatusCodeForHistogram(200), 1);
}
TEST_F(CrossOtrWebContentsObserverTest,
       RefreshedAfterNavigationNoParamsFiltering) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  observer->WebContentsDestroyed();

  histogram_tester.ExpectTotalCount(kCrossOtrRefreshCountMetricName, 0);
  ASSERT_EQ(histogram_tester.GetTotalSum(kCrossOtrRefreshCountMetricName), 0);
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      0);
}
TEST_F(CrossOtrWebContentsObserverTest, UncommittedNavigationWithRefresh) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());

  // Finish a non-reload navigation, but one that isn't committed (so no actual
  // navigation away from the monitored page)
  handle->set_reload_type(content::ReloadType::NONE);
  handle->set_is_in_primary_main_frame(true);
  handle->set_is_same_document(false);
  handle->set_has_committed(false);

  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // We just observed another navigation not due to a client redirect, so should
  // no longer be in the cross-OTR state.
  ASSERT_FALSE(observer->IsCrossOtrState());

  // After that uncommitted navigation, trigger a redirect, then destroy.
  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  observer->WebContentsDestroyed();

  // We had 1 relevant refresh.
  histogram_tester.ExpectTotalCount(kCrossOtrRefreshCountMetricName, 1);
  ASSERT_EQ(histogram_tester.GetTotalSum(kCrossOtrRefreshCountMetricName), 1);
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
}
TEST_F(CrossOtrWebContentsObserverTest,
       UncommittedNavigationWithRefreshNoParamsFiltering) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());

  // Finish a non-reload navigation, but one that isn't committed (so no actual
  // navigation away from the monitored page)
  handle->set_reload_type(content::ReloadType::NONE);
  handle->set_is_in_primary_main_frame(true);
  handle->set_is_same_document(false);
  handle->set_has_committed(false);

  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // We just observed another navigation not due to a client redirect, so should
  // no longer be in the cross-OTR state.
  ASSERT_FALSE(observer->IsCrossOtrState());

  // After that uncommitted navigation, trigger a redirect, then destroy.
  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  observer->WebContentsDestroyed();

  // We had 1 relevant refresh.
  histogram_tester.ExpectTotalCount(kCrossOtrRefreshCountMetricName, 0);
  ASSERT_EQ(histogram_tester.GetTotalSum(kCrossOtrRefreshCountMetricName), 0);
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      0);
}
TEST_F(CrossOtrWebContentsObserverTest, MultipleRefreshesAfterNavigation) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  // Reload twice and ensure the count is persisted.
  handle->set_reload_type(content::ReloadType::NORMAL);
  observer->DidStartNavigation(handle.get());
  // With the refresh navigation started, we are no longer in cross-OTR mode.
  ASSERT_FALSE(observer->IsCrossOtrState());
  observer->DidFinishNavigation(handle.get());
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  // Navigating away means no more observer.
  handle->set_reload_type(content::ReloadType::NONE);
  handle->set_is_in_primary_main_frame(true);
  handle->set_is_same_document(false);
  handle->set_has_committed(true);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());

  ASSERT_EQ(CrossOtrWebContentsObserver::FromWebContents(contents), nullptr);

  histogram_tester.ExpectTotalCount(kCrossOtrRefreshCountMetricName, 1);
  ASSERT_EQ(histogram_tester.GetTotalSum(kCrossOtrRefreshCountMetricName), 2);
  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
}
TEST_F(CrossOtrWebContentsObserverTest, RedirectsAfterNavigation) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);

  // Simulate params filtering, making it okay to collect metrics.
  observer->SetDidFilterParams(
      true, ClassificationExperimentStatus::NON_EXPERIMENTAL);

  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // The first navigation has finished, but we remain cross-OTR until either
  // user activation or a non-client redirect navigation begins
  ASSERT_TRUE(observer->IsCrossOtrState());

  // Redirects observed on navigations after the first should not
  // write responses.
  observer->DidStartNavigation(handle.get());

  // A new, non-client redirect navigation began, so we should no longer be
  // filtering.
  ASSERT_FALSE(observer->IsCrossOtrState());

  response = base::MakeRefCounted<net::HttpResponseHeaders>(
      "HTTP/1.1 302 Moved Temporarily");
  handle->set_response_headers(response);
  observer->DidRedirectNavigation(handle.get());

  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 1);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      1);
}
TEST_F(CrossOtrWebContentsObserverTest,
       RedirectsAfterNavigationNoParamsFiltering) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // The first navigation has finished, but we remain cross-OTR until either
  // user activation or a non-client redirect navigation begins
  ASSERT_TRUE(observer->IsCrossOtrState());

  // Redirects observed on navigations after the first should not
  // write responses.
  observer->DidStartNavigation(handle.get());

  // A new, non-client redirect navigation began, so we should no longer be
  // filtering.
  ASSERT_FALSE(observer->IsCrossOtrState());

  response = base::MakeRefCounted<net::HttpResponseHeaders>(
      "HTTP/1.1 302 Moved Temporarily");
  handle->set_response_headers(response);
  observer->DidRedirectNavigation(handle.get());

  histogram_tester.ExpectTotalCount(kResponseCodeMetricName, 0);
  histogram_tester.ExpectUniqueSample(
      kResponseCodeMetricName, net::HttpUtil::MapStatusCodeForHistogram(200),
      0);
}
TEST_F(CrossOtrWebContentsObserverTest, ClientRedirectCrossOtr) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // The first navigation has finished, but we remain cross-OTR until either
  // user activation or a non-client redirect navigation begins
  ASSERT_TRUE(observer->IsCrossOtrState());

  handle->set_page_transition(ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  observer->DidStartNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());
  observer->DidFinishNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());

  handle->set_page_transition(ui::PAGE_TRANSITION_AUTO_BOOKMARK);
  observer->DidStartNavigation(handle.get());
  // A new, non-client redirect navigation began, so we should no longer be
  // filtering.
  ASSERT_FALSE(observer->IsCrossOtrState());
}
TEST_F(CrossOtrWebContentsObserverTest,
       ClientRedirectAfterActivationNotCrossOtr) {
  base::HistogramTester histogram_tester;
  content::WebContents* contents = web_contents();
  CrossOtrWebContentsObserver::MaybeCreateForWebContents(
      contents, /*is_cross_otr=*/true,
      /*started_from_context_menu=*/true, ui::PAGE_TRANSITION_LINK);
  CrossOtrWebContentsObserver* observer =
      CrossOtrWebContentsObserver::FromWebContents(contents);
  ASSERT_NE(observer, nullptr);
  std::unique_ptr<content::MockNavigationHandle> handle =
      std::make_unique<NiceMock<content::MockNavigationHandle>>(contents);

  scoped_refptr<net::HttpResponseHeaders> response =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  handle->set_response_headers(response);
  observer->DidStartNavigation(handle.get());
  observer->DidFinishNavigation(handle.get());
  // The first navigation has finished, but we remain cross-OTR until either
  // user activation or a non-client redirect navigation begins
  ASSERT_TRUE(observer->IsCrossOtrState());

  handle->set_page_transition(ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  observer->DidStartNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());
  observer->DidFinishNavigation(handle.get());
  ASSERT_TRUE(observer->IsCrossOtrState());

  observer->FrameReceivedUserActivation(nullptr);
  // Receiving user activation means we leave the cross-OTR state and instead
  // allow client redirects to occur unfiltered.
  ASSERT_FALSE(observer->IsCrossOtrState());
  handle->set_page_transition(ui::PAGE_TRANSITION_AUTO_BOOKMARK);
  observer->DidStartNavigation(handle.get());
  // The client redirect should not reset the OTR state.
  ASSERT_FALSE(observer->IsCrossOtrState());
}
}  // namespace url_param_filter
