// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flags.h"

#include "base/command_line.h"
#include "base/metrics/field_trial_params.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"

namespace content {

bool IsFedCmAutoSigninEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      features::kFedCm, features::kFedCmAutoSigninFieldTrialParamName, false);
}

bool IsFedCmIdpSignoutEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      features::kFedCm, features::kFedCmIdpSignoutFieldTrialParamName, false);
}

bool IsFedCmManifestValidationEnabled() {
  return base::FeatureList::IsEnabled(features::kFedCmManifestValidation);
}

bool IsFedCmMultipleIdentityProvidersEnabled() {
  return base::FeatureList::IsEnabled(
      features::kFedCmMultipleIdentityProviders);
}

bool IsFedCmIdpSigninStatusEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      features::kFedCm, features::kFedCmIdpSigninStatusFieldTrialParamName,
      false);
}

bool IsFedCmIframeSupportEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      features::kFedCm, features::kFedCmIframeSupportFieldTrialParamName,
      false);
}

bool IsFedCmMetricsEndpointEnabled() {
  return base::FeatureList::IsEnabled(features::kFedCmMetricsEndpoint);
}

}  // namespace content
