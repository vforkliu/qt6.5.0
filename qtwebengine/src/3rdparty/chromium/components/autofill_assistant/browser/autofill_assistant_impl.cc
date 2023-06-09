// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill_assistant/browser/autofill_assistant_impl.h"

#include <memory>
#include <vector>

#include "base/ranges/algorithm.h"
#include "base/time/default_tick_clock.h"
#include "components/autofill/core/common/signatures.h"
#include "components/autofill_assistant/browser/common_dependencies.h"
#include "components/autofill_assistant/browser/desktop/starter_delegate_desktop.h"
#include "components/autofill_assistant/browser/headless/client_headless.h"
#include "components/autofill_assistant/browser/headless/headless_script_controller_impl.h"
#include "components/autofill_assistant/browser/protocol_utils.h"
#include "components/autofill_assistant/browser/public/password_change/website_login_manager.h"
#include "components/autofill_assistant/browser/service.pb.h"
#include "components/autofill_assistant/browser/service/api_key_fetcher.h"
#include "components/autofill_assistant/browser/service/cup_impl.h"
#include "components/autofill_assistant/browser/service/server_url_fetcher.h"
#include "components/autofill_assistant/browser/service/service_request_sender.h"
#include "components/autofill_assistant/browser/service/service_request_sender_impl.h"
#include "components/autofill_assistant/browser/service/simple_url_loader_factory.h"
#include "components/autofill_assistant/browser/starter.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "net/http/http_status_code.h"

namespace autofill_assistant {

namespace {

const char kIntentScriptParameterKey[] = "INTENT";

void OnCapabilitiesResponse(
    AutofillAssistant::GetCapabilitiesResponseCallback callback,
    int http_status,
    const std::string& response_str,
    const ServiceRequestSender::ResponseInfo& response_info) {
  std::vector<AutofillAssistant::CapabilitiesInfo> infos;
  GetCapabilitiesByHashPrefixResponseProto resp;

  if (http_status != net::HTTP_OK) {
    VLOG(1) << "Failed to get script capabilities."
            << ", http-status=" << http_status;
    // TODO(b/209429727) Record network failure metrics.
    std::move(callback).Run(http_status, infos);
    return;
  }

  if (!resp.ParseFromString(response_str)) {
    LOG(ERROR) << __func__ << " returned unparsable response";
    // TODO(b/209429727) Record parsing failure metrics.
    std::move(callback).Run(http_status, infos);
    return;
  }

  for (const auto& match : resp.match_info()) {
    AutofillAssistant::CapabilitiesInfo info;
    info.url = match.url_match();

    for (const auto& param : match.script_parameters_override()) {
      info.script_parameters[param.name()] = param.value();
    }

    if (match.has_bundle_capabilities_information()) {
      info.bundle_capabilities_information =
          AutofillAssistant::BundleCapabilitiesInformation();

      if (match.bundle_capabilities_information().has_chrome_fast_checkout() &&
          !match.bundle_capabilities_information()
               .chrome_fast_checkout()
               .trigger_form_signatures()
               .empty()) {
        // Source and the target vector are abbreviated due to their length.
        auto& source = match.bundle_capabilities_information()
                           .chrome_fast_checkout()
                           .trigger_form_signatures();

        std::vector<autofill::FormSignature>& target =
            info.bundle_capabilities_information.value()
                .trigger_form_signatures;

        target.reserve(source.size());
        base::ranges::transform(source, std::back_inserter(target),
                                [](uint64_t signature) {
                                  return autofill::FormSignature(signature);
                                });
      }

      if (match.bundle_capabilities_information()
              .has_supports_consentless_execution()) {
        info.bundle_capabilities_information.value()
            .supports_consentless_execution =
            match.bundle_capabilities_information()
                .supports_consentless_execution();
      }
    }

    infos.push_back(info);
  }
  std::move(callback).Run(http_status, infos);
}

}  // namespace

// static
std::unique_ptr<AutofillAssistantImpl> AutofillAssistantImpl::Create(
    content::BrowserContext* browser_context,
    std::unique_ptr<CommonDependencies> dependencies) {
  auto request_sender = std::make_unique<ServiceRequestSenderImpl>(
      browser_context,
      /* access_token_fetcher = */ nullptr,
      std::make_unique<cup::CUPImplFactory>(),
      std::make_unique<NativeURLLoaderFactory>(),
      ApiKeyFetcher().GetAPIKey(dependencies->GetChannel()));
  const ServerUrlFetcher& url_fetcher =
      ServerUrlFetcher(ServerUrlFetcher::GetDefaultServerUrl());

  return std::make_unique<AutofillAssistantImpl>(
      browser_context, std::move(request_sender), std::move(dependencies),
      url_fetcher.GetCapabilitiesByHashEndpoint());
}

AutofillAssistantImpl::AutofillAssistantImpl(
    content::BrowserContext* browser_context,
    std::unique_ptr<ServiceRequestSender> request_sender,
    std::unique_ptr<CommonDependencies> dependencies,
    const GURL& script_server_url)
    : browser_context_(browser_context),
      request_sender_(std::move(request_sender)),
      script_server_url_(script_server_url),
      dependencies_(std::move(dependencies)) {}

AutofillAssistantImpl::~AutofillAssistantImpl() = default;

void AutofillAssistantImpl::GetCapabilitiesByHashPrefix(
    uint32_t hash_prefix_length,
    const std::vector<uint64_t>& hash_prefixes,
    const std::string& intent,
    GetCapabilitiesResponseCallback callback) {
  // Always return an empty response for supervised users.
  if (dependencies_->IsSupervisedUser()) {
    std::move(callback).Run(net::HTTP_OK, {});
    return;
  }

  const ScriptParameters& parameters = {
      base::flat_map<std::string, std::string>{
          {kIntentScriptParameterKey, intent}}};

  ClientContextProto client_context;
  client_context.set_country(dependencies_->GetLatestCountryCode());
  client_context.set_locale(dependencies_->GetLocale());
  client_context.mutable_chrome()->set_chrome_version(
      version_info::GetProductNameAndVersionForUserAgent());

#if BUILDFLAG(IS_ANDROID)
  client_context.set_platform_type(ClientContextProto::PLATFORM_TYPE_ANDROID);
#endif
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_MAC) || \
    BUILDFLAG(IS_WIN) || BUILDFLAG(IS_FUCHSIA)
  client_context.set_platform_type(ClientContextProto::PLATFORM_TYPE_DESKTOP);
#endif

  request_sender_->SendRequest(
      script_server_url_,
      ProtocolUtils::CreateCapabilitiesByHashRequest(
          hash_prefix_length, hash_prefixes, client_context, parameters),
      ServiceRequestSender::AuthMode::API_KEY,
      base::BindOnce(&OnCapabilitiesResponse, std::move(callback)),
      RpcType::GET_CAPABILITIES_BY_HASH_PREFIX);
}

std::unique_ptr<HeadlessScriptController>
AutofillAssistantImpl::CreateHeadlessScriptController(
    content::WebContents* web_contents,
    ExternalActionDelegate* action_extension_delegate,
    WebsiteLoginManager* website_login_manager) {
  auto* starter = Starter::FromWebContents(web_contents);
  if (!starter) {
    return nullptr;
  }

  auto client = std::make_unique<ClientHeadless>(
      web_contents, starter->GetCommonDependencies(), action_extension_delegate,
      website_login_manager, base::DefaultTickClock::GetInstance(),
      RuntimeManager::GetForWebContents(web_contents)->GetWeakPtr(),
      ukm::UkmRecorder::Get(),
      starter->GetCommonDependencies()->GetOrCreateAnnotateDomModelService());
  return std::make_unique<HeadlessScriptControllerImpl>(web_contents, starter,
                                                        std::move(client));
}

}  // namespace autofill_assistant
