// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_SERVICE_MOCK_SERVICE_H_
#define COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_SERVICE_MOCK_SERVICE_H_

#include <string>
#include <vector>

#include "components/autofill_assistant/browser/service/service.h"
#include "components/autofill_assistant/browser/user_data.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace autofill_assistant {

class MockService : public Service {
 public:
  MockService();
  ~MockService() override;

  MOCK_METHOD(void,
              GetScriptsForUrl,
              (const GURL& url,
               const TriggerContext& trigger_context,
               ServiceRequestSender::ResponseCallback callback),
              (override));
  MOCK_METHOD(void,
              GetActions,
              (const std::string& script_path,
               const GURL& url,
               const TriggerContext& trigger_context,
               const std::string& global_payload,
               const std::string& script_payload,
               ServiceRequestSender::ResponseCallback callback),
              (override));
  MOCK_METHOD(void,
              GetNextActions,
              (const TriggerContext& trigger_context,
               const std::string& previous_global_payload,
               const std::string& previous_script_payload,
               const std::vector<ProcessedActionProto>& processed_actions,
               const RoundtripTimingStats& timing_stats,
               const RoundtripNetworkStats& network_stats,
               ServiceRequestSender::ResponseCallback callback),
              (override));
  MOCK_METHOD(void,
              SetScriptStoreConfig,
              (const ScriptStoreConfig& script_store_config),
              (override));
  MOCK_METHOD(void,
              GetUserData,
              (const CollectUserDataOptions& options,
               uint64_t run_id,
               const UserData* user_data,
               ServiceRequestSender::ResponseCallback callback),
              (override));
  MOCK_METHOD(void,
              SetDisableRpcSigning,
              (bool disable_rpc_signing),
              (override));
  MOCK_METHOD(void,
              UpdateAnnotateDomModelContext,
              (int64_t model_version),
              (override));
  MOCK_METHOD(void,
              UpdateJsFlowLibraryLoaded,
              (bool js_flow_library_loaded),
              (override));

  MOCK_METHOD(void,
              ReportProgress,
              (const std::string& token,
               const std::string& payload,
               ServiceRequestSender::ResponseCallback callback),
              (override));
};

}  // namespace autofill_assistant

#endif  // COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_SERVICE_MOCK_SERVICE_H_
