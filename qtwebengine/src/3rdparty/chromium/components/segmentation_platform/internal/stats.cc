// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/segmentation_platform/internal/stats.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/strcat.h"
#include "components/segmentation_platform/public/constants.h"
#include "components/segmentation_platform/public/proto/segmentation_platform.pb.h"
#include "components/segmentation_platform/public/proto/types.pb.h"

namespace segmentation_platform::stats {
namespace {

// Keep in sync with AdaptiveToolbarButtonVariant in enums.xml.
enum class AdaptiveToolbarButtonVariant {
  kUnknown = 0,
  kNone = 1,
  kNewTab = 2,
  kShare = 3,
  kVoice = 4,
  kMaxValue = kVoice,
};

// This is the segmentation subset of
// proto::SegmentId.
// Keep in sync with SegmentationPlatformSegmentationModel in
// //tools/metrics/histograms/enums.xml.
// See also SegmentationModel variant in
// //tools/metrics/histograms/metadata/segmentation_platform/histograms.xml.
enum class SegmentationModel {
  kUnknown = 0,
  kNewTab = 4,
  kShare = 5,
  kVoice = 6,
  kDummy = 10,
  kChromeStartAndroid = 11,
  kQueryTiles = 12,
  kChromeLowUserEngagement = 16,
  kFeedUserSegment = 17,
  kContextualPageActionPriceTracking = 18,
  kChromeStartAndroidV2 = 22,
  kSearchUserSegment = 23,
  kMaxValue = kSearchUserSegment,
};

AdaptiveToolbarButtonVariant OptimizationTargetToAdaptiveToolbarButtonVariant(
    SegmentId segment_id) {
  switch (segment_id) {
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
      return AdaptiveToolbarButtonVariant::kNewTab;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
      return AdaptiveToolbarButtonVariant::kShare;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
      return AdaptiveToolbarButtonVariant::kVoice;
    case SegmentId::OPTIMIZATION_TARGET_UNKNOWN:
      return AdaptiveToolbarButtonVariant::kNone;
    default:
      NOTREACHED();
      return AdaptiveToolbarButtonVariant::kUnknown;
  }
}

bool IsBooleanSegment(const std::string& segmentation_key) {
  // Please keep in sync with BooleanModel variant in
  // //tools/metrics/histograms/metadata/segmentation_platform/histograms.xml.
  return segmentation_key == kChromeStartAndroidSegmentationKey ||
         segmentation_key == kChromeStartAndroidV2SegmentationKey ||
         segmentation_key == kQueryTilesSegmentationKey ||
         segmentation_key == kChromeLowUserEngagementSegmentationKey ||
         segmentation_key == kFeedUserSegmentationKey ||
         segmentation_key == kPowerUserKey ||
         segmentation_key == kShoppingUserSegmentationKey ||
         segmentation_key == kCrossDeviceUserKey ||
         segmentation_key == kFrequentFeatureUserKey ||
         segmentation_key == kIntentionalUserKey ||
         segmentation_key == kResumeHeavyUserKey ||
         segmentation_key == kSearchUserKey;
}

BooleanSegmentSwitch GetBooleanSegmentSwitch(SegmentId new_selection,
                                             SegmentId previous_selection) {
  if (new_selection != SegmentId::OPTIMIZATION_TARGET_UNKNOWN &&
      previous_selection == SegmentId::OPTIMIZATION_TARGET_UNKNOWN) {
    return BooleanSegmentSwitch::kNoneToEnabled;
  } else if (new_selection == SegmentId::OPTIMIZATION_TARGET_UNKNOWN &&
             previous_selection != SegmentId::OPTIMIZATION_TARGET_UNKNOWN) {
    return BooleanSegmentSwitch::kEnabledToNone;
  }
  return BooleanSegmentSwitch::kUnknown;
}

AdaptiveToolbarSegmentSwitch GetAdaptiveToolbarSegmentSwitch(
    SegmentId new_selection,
    SegmentId previous_selection) {
  switch (previous_selection) {
    case SegmentId::OPTIMIZATION_TARGET_UNKNOWN:
      switch (new_selection) {
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
          return AdaptiveToolbarSegmentSwitch::kNoneToNewTab;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
          return AdaptiveToolbarSegmentSwitch::kNoneToShare;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
          return AdaptiveToolbarSegmentSwitch::kNoneToVoice;
        default:
          NOTREACHED();
          return AdaptiveToolbarSegmentSwitch::kUnknown;
      }

    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
      switch (new_selection) {
        case SegmentId::OPTIMIZATION_TARGET_UNKNOWN:
          return AdaptiveToolbarSegmentSwitch::kNewTabToNone;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
          return AdaptiveToolbarSegmentSwitch::kNewTabToShare;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
          return AdaptiveToolbarSegmentSwitch::kNewTabToVoice;
        default:
          NOTREACHED();
          return AdaptiveToolbarSegmentSwitch::kUnknown;
      }

    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
      switch (new_selection) {
        case SegmentId::OPTIMIZATION_TARGET_UNKNOWN:
          return AdaptiveToolbarSegmentSwitch::kShareToNone;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
          return AdaptiveToolbarSegmentSwitch::kShareToNewTab;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
          return AdaptiveToolbarSegmentSwitch::kShareToVoice;
        default:
          NOTREACHED();
          return AdaptiveToolbarSegmentSwitch::kUnknown;
      }

    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
      switch (new_selection) {
        case SegmentId::OPTIMIZATION_TARGET_UNKNOWN:
          return AdaptiveToolbarSegmentSwitch::kVoiceToNone;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
          return AdaptiveToolbarSegmentSwitch::kVoiceToNewTab;
        case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
          return AdaptiveToolbarSegmentSwitch::kVoiceToShare;
        default:
          NOTREACHED();
          return AdaptiveToolbarSegmentSwitch::kUnknown;
      }

    default:
      NOTREACHED();
      return AdaptiveToolbarSegmentSwitch::kUnknown;
  }
}

SegmentationModel OptimizationTargetToSegmentationModel(SegmentId segment_id) {
  switch (segment_id) {
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
      return SegmentationModel::kNewTab;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
      return SegmentationModel::kShare;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
      return SegmentationModel::kVoice;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_DUMMY:
      return SegmentationModel::kDummy;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_START_ANDROID:
      return SegmentationModel::kChromeStartAndroid;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_QUERY_TILES:
      return SegmentationModel::kQueryTiles;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_LOW_USER_ENGAGEMENT:
      return SegmentationModel::kChromeLowUserEngagement;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_FEED_USER:
      return SegmentationModel::kFeedUserSegment;
    case SegmentId::OPTIMIZATION_TARGET_CONTEXTUAL_PAGE_ACTION_PRICE_TRACKING:
      return SegmentationModel::kContextualPageActionPriceTracking;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_START_ANDROID_V2:
      return SegmentationModel::kChromeStartAndroidV2;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SEARCH_USER:
      return SegmentationModel::kSearchUserSegment;
    default:
      return SegmentationModel::kUnknown;
  }
}

// Should map to ModelExecutionStatus variant string in
// //tools/metrics/histograms/metadata/segmentation_platform/histograms.xml.
absl::optional<base::StringPiece> ModelExecutionStatusToHistogramVariant(
    ModelExecutionStatus status) {
  switch (status) {
    case ModelExecutionStatus::kSuccess:
      return "Success";
    case ModelExecutionStatus::kExecutionError:
      return "ExecutionError";

    // Only record duration histograms when tflite model is executed. These
    // cases mean the execution was skipped.
    case ModelExecutionStatus::kSkippedInvalidMetadata:
    case ModelExecutionStatus::kSkippedModelNotReady:
    case ModelExecutionStatus::kSkippedHasFreshResults:
    case ModelExecutionStatus::kSkippedNotEnoughSignals:
    case ModelExecutionStatus::kSkippedResultNotExpired:
    case ModelExecutionStatus::kFailedToSaveResultAfterSuccess:
      return absl::nullopt;
  }
}

// Should map to SignalType variant string in
// //tools/metrics/histograms/metadata/segmentation_platform/histograms.xml.
std::string SignalTypeToHistogramVariant(proto::SignalType signal_type) {
  switch (signal_type) {
    case proto::SignalType::USER_ACTION:
      return "UserAction";
    case proto::SignalType::HISTOGRAM_ENUM:
      return "HistogramEnum";
    case proto::SignalType::HISTOGRAM_VALUE:
      return "HistogramValue";
    default:
      NOTREACHED();
      return "Unknown";
  }
}

float ZeroValueFraction(const std::vector<float>& tensor) {
  if (tensor.size() == 0)
    return 0;

  size_t zero_values = 0;
  for (float feature : tensor) {
    if (feature == 0)
      ++zero_values;
  }
  return static_cast<float>(zero_values) / static_cast<float>(tensor.size());
}

}  // namespace

void RecordModelScore(SegmentId segment_id, float score) {
  // Special case adaptive toolbar models since it already has histograms being
  // recorded and updating names will affect current work.
  switch (segment_id) {
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
      base::UmaHistogramPercentage(
          "SegmentationPlatform.AdaptiveToolbar.ModelScore." +
              SegmentIdToHistogramVariant(segment_id),
          score * 100);
      break;
    default:
      break;
  }

  switch (segment_id) {
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_VOICE:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_NEW_TAB:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SHARE:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_DUMMY:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_START_ANDROID:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_QUERY_TILES:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_CHROME_LOW_USER_ENGAGEMENT:
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_FEED_USER:
    case SegmentId::OPTIMIZATION_TARGET_CONTEXTUAL_PAGE_ACTION_PRICE_TRACKING:
      // This block assumes all models return score between 0 and 1.
      base::UmaHistogramPercentage("SegmentationPlatform.ModelScore." +
                                       SegmentIdToHistogramVariant(segment_id),
                                   score * 100);
      break;
    case SegmentId::OPTIMIZATION_TARGET_SEGMENTATION_SEARCH_USER:
      // This block assumes all models return score between 0 and 100.
      base::UmaHistogramPercentage("SegmentationPlatform.ModelScore." +
                                       SegmentIdToHistogramVariant(segment_id),
                                   base::ClampRound(score));
      break;
    default:
      break;
  }
}

void RecordModelUpdateTimeDifference(SegmentId segment_id,
                                     int64_t model_update_time) {
  // |model_update_time| might be empty for data persisted before M101.
  if (model_update_time) {
    base::Time model_updated_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Seconds(model_update_time));
    base::UmaHistogramCounts1000(
        "SegmentationPlatform.Init.ModelUpdatedTimeDifferenceInDays." +
            SegmentIdToHistogramVariant(segment_id),
        (base::Time::Now() - model_updated_time).InDays());
  }
}

void RecordSegmentSelectionComputed(
    const Config& config,
    SegmentId new_selection,
    absl::optional<SegmentId> previous_selection) {
  // Special case adaptive toolbar since it already has histograms being
  // recorded and updating names will affect current work.
  if (config.segmentation_key == kAdaptiveToolbarSegmentationKey) {
    base::UmaHistogramEnumeration(
        "SegmentationPlatform.AdaptiveToolbar.SegmentSelection.Computed",
        OptimizationTargetToAdaptiveToolbarButtonVariant(new_selection));
  }
  std::string computed_hist =
      base::StrCat({"SegmentationPlatform.", config.segmentation_uma_name,
                    ".SegmentSelection.Computed2"});
  base::UmaHistogramEnumeration(
      computed_hist, OptimizationTargetToSegmentationModel(new_selection));

  SegmentId prev_segment = previous_selection.has_value()
                               ? previous_selection.value()
                               : SegmentId::OPTIMIZATION_TARGET_UNKNOWN;

  if (prev_segment == new_selection || config.on_demand_execution)
    return;

  std::string switched_hist =
      base::StrCat({"SegmentationPlatform.", config.segmentation_uma_name,
                    ".SegmentSwitched"});
  if (config.segmentation_key == kAdaptiveToolbarSegmentationKey) {
    base::UmaHistogramEnumeration(
        switched_hist,
        GetAdaptiveToolbarSegmentSwitch(new_selection, prev_segment));
  } else if (IsBooleanSegment(config.segmentation_key)) {
    base::UmaHistogramEnumeration(
        switched_hist, GetBooleanSegmentSwitch(new_selection, prev_segment));
  }
  // Do not record switched histogram for all keys by default, the client needs
  // to write custom logic for other kinds of segments.
}

void RecordMaintenanceCleanupSignalSuccessCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_1000(
      "SegmentationPlatform.Maintenance.CleanupSignalSuccessCount", count);
}

void RecordMaintenanceCompactionResult(proto::SignalType signal_type,
                                       bool success) {
  base::UmaHistogramBoolean(
      "SegmentationPlatform.Maintenance.CompactionResult." +
          SignalTypeToHistogramVariant(signal_type),
      success);
}

void RecordMaintenanceSignalIdentifierCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_1000(
      "SegmentationPlatform.Maintenance.SignalIdentifierCount", count);
}

void RecordModelDeliveryHasMetadata(SegmentId segment_id, bool has_metadata) {
  base::UmaHistogramBoolean("SegmentationPlatform.ModelDelivery.HasMetadata." +
                                SegmentIdToHistogramVariant(segment_id),
                            has_metadata);
}

void RecordModelDeliveryMetadataFeatureCount(SegmentId segment_id,
                                             size_t count) {
  base::UmaHistogramCounts1000(
      "SegmentationPlatform.ModelDelivery.Metadata.FeatureCount." +
          SegmentIdToHistogramVariant(segment_id),
      count);
}

void RecordModelDeliveryMetadataValidation(
    SegmentId segment_id,
    bool processed,
    metadata_utils::ValidationResult validation_result) {
  // Should map to ValidationPhase variant string in
  // //tools/metrics/histograms/metadata/segmentation_platform/histograms.xml.
  std::string validation_phase = processed ? "Processed" : "Incoming";
  base::UmaHistogramEnumeration(
      "SegmentationPlatform.ModelDelivery.Metadata.Validation." +
          validation_phase + "." + SegmentIdToHistogramVariant(segment_id),
      validation_result);
}

void RecordModelDeliveryReceived(SegmentId segment_id) {
  UMA_HISTOGRAM_ENUMERATION("SegmentationPlatform.ModelDelivery.Received",
                            OptimizationTargetToSegmentationModel(segment_id));
}

void RecordModelDeliverySaveResult(SegmentId segment_id, bool success) {
  base::UmaHistogramBoolean("SegmentationPlatform.ModelDelivery.SaveResult." +
                                SegmentIdToHistogramVariant(segment_id),
                            success);
}

void RecordModelDeliverySegmentIdMatches(SegmentId segment_id, bool matches) {
  base::UmaHistogramBoolean(
      "SegmentationPlatform.ModelDelivery.SegmentIdMatches." +
          SegmentIdToHistogramVariant(segment_id),
      matches);
}

void RecordModelExecutionDurationFeatureProcessing(SegmentId segment_id,
                                                   base::TimeDelta duration) {
  base::UmaHistogramTimes(
      "SegmentationPlatform.ModelExecution.Duration.FeatureProcessing." +
          SegmentIdToHistogramVariant(segment_id),
      duration);
}

void RecordModelExecutionDurationModel(SegmentId segment_id,
                                       bool success,
                                       base::TimeDelta duration) {
  ModelExecutionStatus status = success ? ModelExecutionStatus::kSuccess
                                        : ModelExecutionStatus::kExecutionError;
  absl::optional<base::StringPiece> status_variant =
      ModelExecutionStatusToHistogramVariant(status);
  if (!status_variant)
    return;
  base::UmaHistogramTimes(
      base::StrCat({"SegmentationPlatform.ModelExecution.Duration.Model.",
                    SegmentIdToHistogramVariant(segment_id), ".",
                    *status_variant}),
      duration);
}

void RecordModelExecutionDurationTotal(SegmentId segment_id,
                                       ModelExecutionStatus status,
                                       base::TimeDelta duration) {
  absl::optional<base::StringPiece> status_variant =
      ModelExecutionStatusToHistogramVariant(status);
  if (!status_variant)
    return;
  base::UmaHistogramTimes(
      base::StrCat({"SegmentationPlatform.ModelExecution.Duration.Total.",
                    SegmentIdToHistogramVariant(segment_id), ".",
                    *status_variant}),
      duration);
}

void RecordOnDemandSegmentSelectionDuration(
    const std::string& segmentation_key,
    const SegmentSelectionResult& result,
    base::TimeDelta duration) {
  std::string histogram_prefix =
      base::StrCat({"SegmentationPlatform.SegmentSelectionOnDemand.Duration.",
                    SegmentationKeyToUmaName(segmentation_key), "."});
  base::UmaHistogramTimes(base::StrCat({histogram_prefix, "Any"}), duration);

  std::string histogram_name =
      base::StrCat({histogram_prefix,
                    result.segment.has_value()
                        ? SegmentIdToHistogramVariant(result.segment.value())
                        : "None"});
  base::UmaHistogramTimes(histogram_name, duration);
}

void RecordModelExecutionResult(SegmentId segment_id, float result) {
  base::UmaHistogramPercentage("SegmentationPlatform.ModelExecution.Result." +
                                   SegmentIdToHistogramVariant(segment_id),
                               result * 100);
}

void RecordModelExecutionSaveResult(SegmentId segment_id, bool success) {
  base::UmaHistogramBoolean("SegmentationPlatform.ModelExecution.SaveResult." +
                                SegmentIdToHistogramVariant(segment_id),
                            success);
}

void RecordModelExecutionStatus(SegmentId segment_id,
                                bool default_provider,
                                ModelExecutionStatus status) {
  if (!default_provider) {
    base::UmaHistogramEnumeration(
        "SegmentationPlatform.ModelExecution.Status." +
            SegmentIdToHistogramVariant(segment_id),
        status);
  } else {
    base::UmaHistogramEnumeration(
        "SegmentationPlatform.ModelExecution.DefaultProvider.Status." +
            SegmentIdToHistogramVariant(segment_id),
        status);
  }
}

void RecordModelExecutionZeroValuePercent(SegmentId segment_id,
                                          const std::vector<float>& tensor) {
  base::UmaHistogramPercentage(
      "SegmentationPlatform.ModelExecution.ZeroValuePercent." +
          SegmentIdToHistogramVariant(segment_id),
      ZeroValueFraction(tensor) * 100);
}

void RecordSignalDatabaseGetSamplesDatabaseEntryCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_1000(
      "SegmentationPlatform.SignalDatabase.GetSamples.DatabaseEntryCount",
      count);
}

void RecordSignalDatabaseGetSamplesResult(bool success) {
  UMA_HISTOGRAM_BOOLEAN("SegmentationPlatform.SignalDatabase.GetSamples.Result",
                        success);
}

void RecordSignalDatabaseGetSamplesSampleCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_10000(
      "SegmentationPlatform.SignalDatabase.GetSamples.SampleCount", count);
}

void RecordSignalsListeningCount(
    const std::set<uint64_t>& user_actions,
    const std::set<std::pair<std::string, proto::SignalType>>& histograms) {
  uint64_t user_action_count = user_actions.size();
  uint64_t histogram_enum_count = 0;
  uint64_t histogram_value_count = 0;
  for (auto& s : histograms) {
    if (s.second == proto::SignalType::HISTOGRAM_ENUM)
      ++histogram_enum_count;
    if (s.second == proto::SignalType::HISTOGRAM_VALUE)
      ++histogram_value_count;
  }

  base::UmaHistogramCounts1000(
      "SegmentationPlatform.Signals.ListeningCount." +
          SignalTypeToHistogramVariant(proto::SignalType::USER_ACTION),
      user_action_count);
  base::UmaHistogramCounts1000(
      "SegmentationPlatform.Signals.ListeningCount." +
          SignalTypeToHistogramVariant(proto::SignalType::HISTOGRAM_ENUM),
      histogram_enum_count);
  base::UmaHistogramCounts1000(
      "SegmentationPlatform.Signals.ListeningCount." +
          SignalTypeToHistogramVariant(proto::SignalType::HISTOGRAM_VALUE),
      histogram_value_count);
}

void RecordSegmentSelectionFailure(const Config& config,
                                   SegmentationSelectionFailureReason reason) {
  base::UmaHistogramEnumeration(
      base::StrCat({"SegmentationPlatform.SelectionFailedReason.",
                    config.segmentation_uma_name}),
      reason);
}

std::string FeatureProcessingErrorToString(FeatureProcessingError error) {
  switch (error) {
    case FeatureProcessingError::kUkmEngineDisabled:
      return "UkmEngineDisabled";
    case FeatureProcessingError::kUmaValidationError:
      return "UmaValidationError";
    case FeatureProcessingError::kSqlValidationError:
      return "SqlValidationError";
    case FeatureProcessingError::kCustomInputError:
      return "CustomInputError";
    case FeatureProcessingError::kSqlBindValuesError:
      return "SqlBindValuesError";
    case FeatureProcessingError::kSqlQueryRunError:
      return "SqlQueryRunError";
    case FeatureProcessingError::kResultTensorError:
      return "ResultTensorError";
    default:
      return "Other";
  }
}

void RecordFeatureProcessingError(SegmentId segment_id,
                                  FeatureProcessingError error) {
  base::UmaHistogramEnumeration(
      "SegmentationPlatform.FeatureProcessing.Error." +
          SegmentIdToHistogramVariant(segment_id),
      error);
}

void RecordModelAvailability(SegmentId segment_id,
                             SegmentationModelAvailability availability) {
  base::UmaHistogramEnumeration("SegmentationPlatform.ModelAvailability." +
                                    SegmentIdToHistogramVariant(segment_id),
                                availability);
}

void RecordTooManyInputTensors(int tensor_size) {
  UMA_HISTOGRAM_COUNTS_100(
      "SegmentationPlatform.StructuredMetrics.TooManyTensors.Count",
      tensor_size);
}

void RecordTrainingDataCollectionEvent(SegmentId segment_id,
                                       TrainingDataCollectionEvent event) {
  base::UmaHistogramEnumeration(
      "SegmentationPlatform.TrainingDataCollectionEvents." +
          SegmentIdToHistogramVariant(segment_id),
      event);
}

}  // namespace segmentation_platform::stats
