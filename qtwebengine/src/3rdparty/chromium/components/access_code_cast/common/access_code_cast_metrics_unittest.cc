// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/access_code_cast/common/access_code_cast_metrics.h"

#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "base/test/metrics/histogram_enum_reader.h"
#include "base/test/metrics/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AccessCodeCastMetricsTest, RecordDialogOpenLocation) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordDialogOpenLocation(
      AccessCodeCastDialogOpenLocation::kBrowserCastMenu);
  histogram_tester.ExpectBucketCount("AccessCodeCast.Ui.DialogOpenLocation", 0,
                                     1);

  AccessCodeCastMetrics::RecordDialogOpenLocation(
      AccessCodeCastDialogOpenLocation::kSystemTrayCastFeaturePod);
  histogram_tester.ExpectBucketCount("AccessCodeCast.Ui.DialogOpenLocation", 1,
                                     1);
  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.DialogOpenLocation", 2);

  AccessCodeCastMetrics::RecordDialogOpenLocation(
      AccessCodeCastDialogOpenLocation::kSystemTrayCastMenu);
  histogram_tester.ExpectBucketCount("AccessCodeCast.Ui.DialogOpenLocation", 2,
                                     1);
  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.DialogOpenLocation", 3);
}

TEST(AccessCodeCastMetricsTest, RecordAddSinkResult) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordAddSinkResult(
      false, AccessCodeCastAddSinkResult::kUnknownError);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.New", 0, 1);
  AccessCodeCastMetrics::RecordAddSinkResult(false,
                                             AccessCodeCastAddSinkResult::kOk);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.New", 1, 1);
  AccessCodeCastMetrics::RecordAddSinkResult(
      false, AccessCodeCastAddSinkResult::kAuthError);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.New", 2, 1);
  histogram_tester.ExpectTotalCount(
      "AccessCodeCast.Discovery.AddSinkResult.New", 3);

  AccessCodeCastMetrics::RecordAddSinkResult(
      true, AccessCodeCastAddSinkResult::kUnknownError);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.Remembered", 0, 1);
  AccessCodeCastMetrics::RecordAddSinkResult(true,
                                             AccessCodeCastAddSinkResult::kOk);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.Remembered", 1, 1);
  AccessCodeCastMetrics::RecordAddSinkResult(
      true, AccessCodeCastAddSinkResult::kAuthError);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.AddSinkResult.Remembered", 2, 1);
  histogram_tester.ExpectTotalCount(
      "AccessCodeCast.Discovery.AddSinkResult.New", 3);
  histogram_tester.ExpectTotalCount(
      "AccessCodeCast.Discovery.AddSinkResult.Remembered", 3);
}

TEST(AccessCodeCastMetricsTest, OnCastSessionResult) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::OnCastSessionResult(
      0 /* ResultCode::UNKNOWN_ERROR */, AccessCodeCastCastMode::kPresentation);
  histogram_tester.ExpectTotalCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 0);

  AccessCodeCastMetrics::OnCastSessionResult(
      1 /* RouteRequest::OK */, AccessCodeCastCastMode::kPresentation);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 0, 1);
  AccessCodeCastMetrics::OnCastSessionResult(
      1 /* RouteRequest::OK */, AccessCodeCastCastMode::kTabMirror);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 1, 1);
  AccessCodeCastMetrics::OnCastSessionResult(
      1 /* RouteRequest::OK */, AccessCodeCastCastMode::kDesktopMirror);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 2, 1);
  AccessCodeCastMetrics::OnCastSessionResult(
      1 /* RouteRequest::OK */, AccessCodeCastCastMode::kRemotePlayback);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 3, 1);
  histogram_tester.ExpectTotalCount(
      "AccessCodeCast.Discovery.CastModeOnSuccess", 4);
}

TEST(AccessCodeCastMetricsTest, RecordAccessCodeNotFoundCount) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordAccessCodeNotFoundCount(0);
  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.AccessCodeNotFoundCount",
                                    0);

  AccessCodeCastMetrics::RecordAccessCodeNotFoundCount(1);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.AccessCodeNotFoundCount", 1, 1);

  AccessCodeCastMetrics::RecordAccessCodeNotFoundCount(100);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.AccessCodeNotFoundCount", 100, 1);

  // Over 100 should be reported as 100.
  AccessCodeCastMetrics::RecordAccessCodeNotFoundCount(500);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.AccessCodeNotFoundCount", 100, 2);

  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.AccessCodeNotFoundCount",
                                    3);
}

TEST(AccessCodeCastMetricsTest, RecordDialogLoadTime) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordDialogLoadTime(base::Milliseconds(10));
  histogram_tester.ExpectBucketCount("AccessCodeCast.Ui.DialogLoadTime", 10, 1);

  // Ten seconds (10,000 ms) is the max for UmaHistogramTimes.
  AccessCodeCastMetrics::RecordDialogLoadTime(base::Seconds(10));
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.DialogLoadTime", 10000, 1);
  AccessCodeCastMetrics::RecordDialogLoadTime(base::Seconds(20));
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.DialogLoadTime", 10000, 2);

  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.DialogLoadTime", 3);
}

TEST(AccessCodeCastMetricsTest, RecordDialogCloseReason) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordDialogCloseReason(
      AccessCodeCastDialogCloseReason::kFocus);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.DialogCloseReason", 0, 1);

  AccessCodeCastMetrics::RecordDialogCloseReason(
      AccessCodeCastDialogCloseReason::kCancel);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.DialogCloseReason", 1, 1);

  AccessCodeCastMetrics::RecordDialogCloseReason(
      AccessCodeCastDialogCloseReason::kCastSuccess);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Ui.DialogCloseReason", 2, 1);

  histogram_tester.ExpectTotalCount("AccessCodeCast.Ui.DialogCloseReason", 3);
}

TEST(AccessCodeCastMetricsTest, RecordRememberedDevicesCount) {
  base::HistogramTester histogram_tester;

  AccessCodeCastMetrics::RecordRememberedDevicesCount(0);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.RememberedDevicesCount", 0, 1);

  AccessCodeCastMetrics::RecordRememberedDevicesCount(1);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.RememberedDevicesCount", 1, 1);

  AccessCodeCastMetrics::RecordRememberedDevicesCount(100);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.RememberedDevicesCount", 100, 1);

  // Over 100 should be reported as 100.
  AccessCodeCastMetrics::RecordRememberedDevicesCount(500);
  histogram_tester.ExpectBucketCount(
      "AccessCodeCast.Discovery.RememberedDevicesCount", 100, 2);
}

TEST(AccessCodeCastMetricsTest, CheckMetricsEnums) {
  base::HistogramTester histogram_tester;

  // AddSinkResult
  absl::optional<base::HistogramEnumEntryMap> add_sink_results =
      base::ReadEnumFromEnumsXml("AccessCodeCastAddSinkResult");
  EXPECT_TRUE(add_sink_results->size() ==
      static_cast<int>(AccessCodeCastAddSinkResult::kMaxValue) + 1)
      << "'AccessCodeCastAddSinkResult' enum was changed in "
         "access_code_cast_metrics.h. Please update the entry in "
         "enums.xml to match.";

  // CastMode
  absl::optional<base::HistogramEnumEntryMap> cast_modes =
      base::ReadEnumFromEnumsXml("AccessCodeCastCastMode");
  EXPECT_TRUE(cast_modes->size() ==
      static_cast<int>(AccessCodeCastCastMode::kMaxValue) + 1)
      << "'AccessCodeCastCastMode' enum was changed in "
         "access_code_cast_metrics.h. Please update the entry in "
         "enums.xml to match.";

  // DialogCloseReason
  absl::optional<base::HistogramEnumEntryMap> dialog_close_reasons =
      base::ReadEnumFromEnumsXml("AccessCodeCastDialogCloseReason");
  EXPECT_TRUE(dialog_close_reasons->size() ==
      static_cast<int>(AccessCodeCastDialogCloseReason::kMaxValue) + 1)
      << "'AccessCodeCastDialogCloseReason' enum was changed in "
         "access_code_cast_metrics.h. Please update the entry in "
         "enums.xml to match.";

  // DialogOpenLocation
  absl::optional<base::HistogramEnumEntryMap> dialog_open_locations =
      base::ReadEnumFromEnumsXml("AccessCodeCastDialogOpenLocation");
  EXPECT_TRUE(dialog_open_locations->size() ==
      static_cast<int>(AccessCodeCastDialogOpenLocation::kMaxValue) + 1)
      << "'AccessCodeCastDialogOpenLocation' enum was changed in "
         "access_code_cast_metrics.h. Please update the entry in "
         "enums.xml to match.";
}
