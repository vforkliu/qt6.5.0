// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROFILE_METRICS_COUNTS_H_
#define COMPONENTS_PROFILE_METRICS_COUNTS_H_

#include "base/metrics/histogram_base.h"

namespace profile_metrics {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class ProfileColorsUniqueness {
  kSingleProfile = 0,
  kUnique = 1,
  kUniqueExceptForRepeatedDefault = 2,
  kRepeated = 3,
  kMaxValue = kRepeated,
};

struct Counts {
  base::HistogramBase::Sample total = 0;
  base::HistogramBase::Sample signedin = 0;
  base::HistogramBase::Sample supervised = 0;
  base::HistogramBase::Sample active = 0;
  base::HistogramBase::Sample unused = 0;
  ProfileColorsUniqueness colors_uniqueness =
      ProfileColorsUniqueness::kRepeated;
};

// Logs metrics related to |counts|.
void LogProfileMetricsCounts(const Counts& counts);

}  // namespace profile_metrics

#endif  // COMPONENTS_PROFILE_METRICS_COUNTS_H_
