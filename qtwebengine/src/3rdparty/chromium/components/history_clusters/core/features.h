// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CLUSTERS_CORE_FEATURES_H_
#define COMPONENTS_HISTORY_CLUSTERS_CORE_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace history_clusters {

// Features

namespace internal {

// Enables Journeys in the Chrome History WebUI. This flag shouldn't be checked
// directly. Instead use `IsJourneysEnabled()` for the system language filter.
BASE_DECLARE_FEATURE(kJourneys);

// Enables labelling of Journeys in UI.
BASE_DECLARE_FEATURE(kJourneysLabels);

// Enables persisting and using persisted clusters.
BASE_DECLARE_FEATURE(kPersistedClusters);

// Enables the Journeys Omnibox Action chip. `kJourneys` must also be enabled
// for this to take effect.
BASE_DECLARE_FEATURE(kOmniboxAction);

// Enables the `HistoryClusterProvider` to surface Journeys as a suggestion row
// instead of an action chip. Enabling this won't actually disable
// `kOmniboxAction` but for user experiments, the intent is to only have 1
// enabled. `kJourneys` must also be enabled for this to take effect.
BASE_DECLARE_FEATURE(kOmniboxHistoryClusterProvider);

// Enables debug info in non-user-visible surfaces, like Chrome Inspector.
// Does nothing if `kJourneys` is disabled.
BASE_DECLARE_FEATURE(kNonUserVisibleDebug);

// Enables debug info in user-visible surfaces, like the actual WebUI page.
// Does nothing if `kJourneys` is disabled.
BASE_DECLARE_FEATURE(kUserVisibleDebug);

// This flag is to enable us to turn on persisting context annotations WITHOUT
// exposing the Journeys UI in general. If EITHER this flag or `kJourneys` is
// enabled, users will have context annotations persisted into their History DB.
BASE_DECLARE_FEATURE(kPersistContextAnnotationsInHistoryDb);

// Enables the history clusters internals page.
BASE_DECLARE_FEATURE(kHistoryClustersInternalsPage);

// Enables use of task runner with trait CONTINUE_ON_SHUTDOWN.
BASE_DECLARE_FEATURE(kHistoryClustersUseContinueOnShutdown);

// Enables use of additional keyword filtering operations on clusters.
BASE_DECLARE_FEATURE(kHistoryClustersKeywordFiltering);

// Order consistently with config.h.

}  // namespace internal

// The below features are NOT internal and NOT encapsulated in the Config class.
// These are different because the base::Feature instance needs to be directly
// referred to outside of Journeys code. Moreover, they are not used inside an
// inner loop, so they don't need to be high performance.

// Enables the user survey when the user clicks over to Journeys from History.
BASE_DECLARE_FEATURE(kJourneysSurveyForHistoryEntrypoint);
extern const base::FeatureParam<base::TimeDelta>
    kJourneysSurveyForHistoryEntrypointDelay;

// Enables the user survey when the user uses the omnibox to access Journeys.
BASE_DECLARE_FEATURE(kJourneysSurveyForOmniboxEntrypoint);
extern const base::FeatureParam<base::TimeDelta>
    kJourneysSurveyForOmniboxEntrypointDelay;

}  // namespace history_clusters

#endif  // COMPONENTS_HISTORY_CLUSTERS_CORE_FEATURES_H_
