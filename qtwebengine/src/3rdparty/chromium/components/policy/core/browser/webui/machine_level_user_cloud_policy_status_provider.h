// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_WEBUI_MACHINE_LEVEL_USER_CLOUD_POLICY_STATUS_PROVIDER_H_
#define COMPONENTS_POLICY_CORE_BROWSER_WEBUI_MACHINE_LEVEL_USER_CLOUD_POLICY_STATUS_PROVIDER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/policy/core/browser/webui/policy_status_provider.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/policy_export.h"

namespace policy {
class CloudPolicyCore;

// The following constants identify top-level keys in the dictionary returned by
// and are specific to MachineLevelUserCloudPolicyStatusProvider.
POLICY_EXPORT extern const char kDeviceIdKey[];
POLICY_EXPORT extern const char kEnrollmentTokenKey[];
POLICY_EXPORT extern const char kMachineKey[];

struct POLICY_EXPORT MachineLevelUserCloudPolicyContext {
  std::string enrollmentToken;
  std::string deviceId;
  base::Time lastCloudReportSent;
};

class POLICY_EXPORT MachineLevelUserCloudPolicyStatusProvider
    : public PolicyStatusProvider,
      public CloudPolicyStore::Observer {
 public:
  MachineLevelUserCloudPolicyStatusProvider(
      CloudPolicyCore* core,
      MachineLevelUserCloudPolicyContext* context);
  MachineLevelUserCloudPolicyStatusProvider(
      const MachineLevelUserCloudPolicyStatusProvider&) = delete;
  MachineLevelUserCloudPolicyStatusProvider& operator=(
      const MachineLevelUserCloudPolicyStatusProvider&) = delete;
  ~MachineLevelUserCloudPolicyStatusProvider() override;

  // PolicyStatusProvider implementation.
  base::Value::Dict GetStatus() override;

  // CloudPolicyStore::Observer implementation.
  void OnStoreLoaded(CloudPolicyStore* store) override;
  void OnStoreError(CloudPolicyStore* store) override;

 private:
  raw_ptr<CloudPolicyCore> core_;
  raw_ptr<MachineLevelUserCloudPolicyContext> context_;
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_WEBUI_MACHINE_LEVEL_USER_CLOUD_POLICY_STATUS_PROVIDER_H_
