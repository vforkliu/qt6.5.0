// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_METRICS_H_
#define CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_METRICS_H_

#include <string>

#include "base/time/time.h"
#include "content/browser/preloading/prerender/prerender_host.h"
#include "content/browser/preloading/prerender/prerender_host_registry.h"
#include "content/browser/renderer_host/render_frame_host_delegate.h"
#include "content/public/browser/prerender_trigger_type.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// Note: Please update GetCancelledInterfaceType() in the corresponding .cc file
// and the enum of PrerenderCancelledUnknownInterface in
// tools/metrics/histograms/enums.xml if you add a new item.
enum class PrerenderCancelledInterface {
  kUnknown = 0,  // For kCancel interfaces added by embedders or tests.
  kGamepadHapticsManager = 1,
  kGamepadMonitor = 2,
  // kNotificationService = 3,   Deprecated.
  kSyncEncryptionKeysExtension = 4,
  kMaxValue = kSyncEncryptionKeysExtension
};

// Used by PrerenderNavigationThrottle, to track the cross-origin cancellation
// reason, and break it down into more cases.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PrerenderCrossOriginRedirectionMismatch {
  kShouldNotBeReported = 0,
  kPortMismatch = 1,
  kHostMismatch = 2,
  kHostPortMismatch = 3,
  kSchemeMismatch = 4,
  kSchemePortMismatch = 5,
  kSchemeHostMismatch = 6,
  kSchemeHostPortMismatch = 7,
  kMaxValue = kSchemeHostPortMismatch
};

// Used by PrerenderNavigationThrottle. This is a breakdown enum for
// PrerenderCrossOriginRedirectionMismatch.kSchemePortMismatch.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PrerenderCrossOriginRedirectionProtocolChange {
  kHttpProtocolUpgrade = 0,
  kHttpProtocolDowngrade = 1,
  kMaxValue = kHttpProtocolDowngrade
};

// Used by PrerenderNavigationThrottle. This is a breakdown enum for
// PrerenderCrossOriginRedirectionMismatch.kHostMismatch.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PrerenderCrossOriginRedirectionDomain {
  kRedirectToSubDomain = 0,
  kRedirectFromSubDomain = 1,
  kCrossDomain = 2,
  kMaxValue = kCrossDomain
};

void RecordPrerenderCancelledInterface(
    const std::string& interface_name,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_histogram_suffix);

void RecordPrerenderReasonForInactivePageRestriction(uint16_t reason,
                                                     RenderFrameHostImpl& rfh);

void RecordPrerenderTriggered(ukm::SourceId ukm_id);

void RecordPrerenderActivationTime(
    base::TimeDelta delta,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_histogram_suffix);

// Records the status to UMA and UKM, and reports the status other than
// kActivated to DevTools. In the attributes, `initiator_ukm_id` represents the
// page that starts prerendering. `prerendered_ukm_id` represents the
// prerendered page and is valid after the page is activated.
void RecordPrerenderHostFinalStatus(PrerenderHost::FinalStatus status,
                                    const PrerenderAttributes& attributes,
                                    ukm::SourceId prerendered_ukm_id);

// Records which navigation parameters are different between activation and
// initial prerender navigation when activation fails.
void RecordPrerenderActivationNavigationParamsMatch(
    PrerenderHost::ActivationNavigationParamsMatch result,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_suffix);

// Records the detailed types of the cross-origin redirection, e.g., changes to
// scheme, host name etc.
void RecordPrerenderRedirectionMismatchType(
    PrerenderCrossOriginRedirectionMismatch case_type,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_histogram_suffix);

// Records whether the redirection was caused by HTTP protocol upgrade.
void RecordPrerenderRedirectionProtocolChange(
    PrerenderCrossOriginRedirectionProtocolChange change_type,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_histogram_suffix);

// Records whether the prerendering navigation was redirected to a subdomain
// page.
void RecordPrerenderRedirectionDomain(
    PrerenderCrossOriginRedirectionDomain domain_type,
    PrerenderTriggerType trigger_type,
    const std::string& embedder_histogram_suffix);

}  // namespace content

#endif  // CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_METRICS_H_
