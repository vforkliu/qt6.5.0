// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/page_info/core/about_this_site_service.h"
#include <memory>
#include "base/memory/raw_ptr.h"
#include "base/test/metrics/histogram_tester.h"
#include "components/optimization_guide/proto/common_types.pb.h"
#include "components/page_info/core/about_this_site_validation.h"
#include "components/page_info/core/features.h"
#include "components/page_info/core/proto/about_this_site_metadata.pb.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace page_info {
using testing::_;
using testing::Invoke;
using testing::Return;

using about_this_site_validation::AboutThisSiteStatus;
using AboutThisSiteInteraction = AboutThisSiteService::AboutThisSiteInteraction;
using optimization_guide::OptimizationGuideDecision;
using optimization_guide::OptimizationMetadata;

class MockAboutThisSiteServiceClient : public AboutThisSiteService::Client {
 public:
  MockAboutThisSiteServiceClient() = default;

  MOCK_METHOD2(CanApplyOptimization,
               OptimizationGuideDecision(const GURL&, OptimizationMetadata*));
};

proto::AboutThisSiteMetadata CreateValidMetadata() {
  proto::AboutThisSiteMetadata metadata;
  auto* description = metadata.mutable_site_info()->mutable_description();
  description->set_description(
      "A domain used in illustrative examples in documents");
  description->set_lang("en_US");
  description->set_name("Example");
  description->mutable_source()->set_url("https://example.com");
  description->mutable_source()->set_label("Example source");
  metadata.mutable_site_info()->mutable_more_about()->set_url(
      "https://google.com/ats/example.com");
  return metadata;
}

OptimizationGuideDecision ReturnDescription(const GURL& url,
                                            OptimizationMetadata* metadata) {
  optimization_guide::proto::Any any_metadata;
  any_metadata.set_type_url(
      "type.googleapis.com/com.foo.AboutThisSiteMetadata");
  CreateValidMetadata().SerializeToString(any_metadata.mutable_value());
  metadata->set_any_metadata(any_metadata);
  return OptimizationGuideDecision::kTrue;
}

OptimizationGuideDecision ReturnInvalidDescription(
    const GURL& url,
    OptimizationMetadata* metadata) {
  optimization_guide::proto::Any any_metadata;
  any_metadata.set_type_url(
      "type.googleapis.com/com.foo.AboutThisSiteMetadata");
  proto::AboutThisSiteMetadata about_this_site_metadata = CreateValidMetadata();
  about_this_site_metadata.mutable_site_info()
      ->mutable_description()
      ->clear_source();
  about_this_site_metadata.SerializeToString(any_metadata.mutable_value());
  metadata->set_any_metadata(any_metadata);
  return OptimizationGuideDecision::kTrue;
}

OptimizationGuideDecision ReturnNoResult(const GURL& url,
                                         OptimizationMetadata* metadata) {
  return OptimizationGuideDecision::kFalse;
}

OptimizationGuideDecision ReturnUnknown(const GURL& url,
                                        OptimizationMetadata* metadata) {
  return OptimizationGuideDecision::kUnknown;
}

class AboutThisSiteServiceTest : public testing::Test {
 public:
  void SetUp() override {
    auto client_mock =
        std::make_unique<testing::StrictMock<MockAboutThisSiteServiceClient>>();
    client_ = client_mock.get();
    service_ = std::make_unique<AboutThisSiteService>(
        std::move(client_mock), /*allow_missing_description*/ false);
  }

  MockAboutThisSiteServiceClient* client() { return client_; }
  AboutThisSiteService* service() { return service_.get(); }

 private:
  raw_ptr<MockAboutThisSiteServiceClient> client_;
  std::unique_ptr<AboutThisSiteService> service_;
};

// Tests that correct proto messages are accepted.
TEST_F(AboutThisSiteServiceTest, ValidResponse) {
  base::HistogramTester t;
  EXPECT_CALL(*client(), CanApplyOptimization(_, _))
      .WillOnce(Invoke(&ReturnDescription));

  auto info = service()->GetAboutThisSiteInfo(
      GURL("https://foo.com"), ukm::UkmRecorder::GetNewSourceID());
  EXPECT_TRUE(info.has_value());
  EXPECT_EQ(info->more_about().url(),
            "https://google.com/ats/example.com?ctx=chrome");
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteStatus",
                       AboutThisSiteStatus::kValid, 1);
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteInteraction",
                       AboutThisSiteInteraction::kShownWithDescription, 1);
}

// Tests the language specific feature check.
TEST_F(AboutThisSiteServiceTest, FeatureCheck) {
  EXPECT_TRUE(page_info::IsAboutThisSiteFeatureEnabled("en-US"));
  EXPECT_TRUE(page_info::IsAboutThisSiteFeatureEnabled("en-GB"));
  EXPECT_TRUE(page_info::IsAboutThisSiteFeatureEnabled("en"));

  EXPECT_FALSE(page_info::IsAboutThisSiteFeatureEnabled("de-DE"));
  EXPECT_FALSE(page_info::IsAboutThisSiteFeatureEnabled("de"));
}

// Tests that incorrect proto messages are discarded.
TEST_F(AboutThisSiteServiceTest, InvalidResponse) {
  base::HistogramTester t;
  EXPECT_CALL(*client(), CanApplyOptimization(_, _))
      .WillOnce(Invoke(&ReturnInvalidDescription));

  auto info = service()->GetAboutThisSiteInfo(
      GURL("https://foo.com"), ukm::UkmRecorder::GetNewSourceID());
  EXPECT_FALSE(info.has_value());
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteStatus",
                       AboutThisSiteStatus::kMissingDescriptionSource, 1);
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteInteraction",
                       AboutThisSiteInteraction::kNotShown, 1);
}

// Tests that no response is handled.
TEST_F(AboutThisSiteServiceTest, NoResponse) {
  base::HistogramTester t;
  EXPECT_CALL(*client(), CanApplyOptimization(_, _))
      .WillOnce(Invoke(&ReturnNoResult));

  auto info = service()->GetAboutThisSiteInfo(
      GURL("https://foo.com"), ukm::UkmRecorder::GetNewSourceID());
  EXPECT_FALSE(info.has_value());
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteStatus",
                       AboutThisSiteStatus::kNoResult, 1);
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteInteraction",
                       AboutThisSiteInteraction::kNotShown, 1);
}

// Tests that unknown response is handled.
TEST_F(AboutThisSiteServiceTest, Unknown) {
  base::HistogramTester t;
  EXPECT_CALL(*client(), CanApplyOptimization(_, _))
      .WillOnce(Invoke(&ReturnUnknown));

  auto info = service()->GetAboutThisSiteInfo(
      GURL("https://foo.com"), ukm::UkmRecorder::GetNewSourceID());
  EXPECT_FALSE(info.has_value());
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteStatus",
                       AboutThisSiteStatus::kUnknown, 1);
  t.ExpectUniqueSample("Security.PageInfo.AboutThisSiteInteraction",
                       AboutThisSiteInteraction::kNotShown, 1);
}

}  // namespace page_info
