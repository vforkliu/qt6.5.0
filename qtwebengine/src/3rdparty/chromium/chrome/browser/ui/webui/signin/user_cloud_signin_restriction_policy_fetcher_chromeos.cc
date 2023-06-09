// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/user_cloud_signin_restriction_policy_fetcher_chromeos.h"

#include "base/command_line.h"
#include "base/json/json_string_value_serializer.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/policy_switches.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace ash {

namespace {

constexpr char kSecondaryGoogleAccountUsageLatencyHistogramName[] =
    "Enterprise.SecondaryGoogleAccountUsage.PolicyFetch.ResponseLatency";

const char kAuthorizationHeaderFormat[] = "Bearer %s";
const char kSecureConnectApiGetSecondaryGoogleAccountUsageUrl[] =
    "https://secureconnect-pa.clients6.google.com/"
    "v1:getManagedAccountsSigninRestriction?policy_name="
    "SecondaryGoogleAccountUsage";
const char kJsonContentType[] = "application/json";
// Presence of this key in the user info response indicates whether the user is
// on a hosted domain.
const char kHostedDomainKey[] = "hd";
constexpr net::NetworkTrafficAnnotationTag kAnnotation =
    net::DefineNetworkTrafficAnnotation(
        "managed_acccount_signin_restrictions_secure_connect_chromeos",
        R"(
    semantics {
      sender: "Chrome OS sign-in restrictions"
      description:
        "A request to the SecureConnect API to retrieve the value of the "
        "SecondaryGoogleAccountUsage policy for the signed in user."
      trigger:
        "After a user signs into a managed account as a secondary account in "
        "Chrome OS."
      data:
        "Gaia access token."
      destination: GOOGLE_OWNED_SERVICE
    }
    policy {
      cookies_allowed: NO
      chrome_policy {
        SigninInterceptionEnabled {
          SigninInterceptionEnabled: false
        }
      }
    })");

std::unique_ptr<network::SimpleURLLoader> CreateUrlLoader(
    const GURL& url,
    const std::string& access_token,
    const net::NetworkTrafficAnnotationTag& annotation) {
  auto resource_request = std::make_unique<network::ResourceRequest>();

  resource_request->url = url;
  resource_request->method = net::HttpRequestHeaders::kGetMethod;
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  resource_request->headers.SetHeader(net::HttpRequestHeaders::kContentType,
                                      kJsonContentType);

  resource_request->headers.SetHeader(
      net::HttpRequestHeaders::kAuthorization,
      base::StringPrintf(kAuthorizationHeaderFormat, access_token.c_str()));
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  return network::SimpleURLLoader::Create(std::move(resource_request),
                                          annotation);
}

}  // namespace

// static
const char UserCloudSigninRestrictionPolicyFetcherChromeOS::
    kSecondaryGoogleAccountUsagePolicyValueAll[] = "all";
// static
const char UserCloudSigninRestrictionPolicyFetcherChromeOS::
    kSecondaryGoogleAccountUsagePolicyValuePrimaryAccountSignin[] =
        "primary_account_signin";

UserCloudSigninRestrictionPolicyFetcherChromeOS::
    UserCloudSigninRestrictionPolicyFetcherChromeOS(
        const std::string& email,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : email_(email), url_loader_factory_(url_loader_factory) {
  DCHECK(url_loader_factory_);
  user_info_fetcher_ =
      std::make_unique<policy::UserInfoFetcher>(this, url_loader_factory_);
}

UserCloudSigninRestrictionPolicyFetcherChromeOS::
    ~UserCloudSigninRestrictionPolicyFetcherChromeOS() = default;

void UserCloudSigninRestrictionPolicyFetcherChromeOS::
    GetSecondaryGoogleAccountUsage(
        std::unique_ptr<OAuth2AccessTokenFetcher> access_token_fetcher,
        PolicyInfoCallback callback) {
  DCHECK(access_token_fetcher);
  DCHECK(callback);
  DCHECK(!callback_) << "A request is already in progress";
  callback_ = std::move(callback);
  if (policy::BrowserPolicyConnector::IsNonEnterpriseUser(email_)) {
    // Non Enterprise accounts do not have restrictions.
    std::move(callback_).Run(/*status=*/Status::kUnsupportedAccountTypeError,
                             /*policy=*/absl::nullopt,
                             /*domain=*/std::string());
    return;
  }
  access_token_fetcher_ = std::move(access_token_fetcher);
  FetchAccessToken();
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::FetchAccessToken() {
  access_token_fetcher_->Start(
      GaiaUrls::GetInstance()->oauth2_chrome_client_id(),
      GaiaUrls::GetInstance()->oauth2_chrome_client_secret(),
      {GaiaConstants::kGoogleUserInfoEmail,
       GaiaConstants::kGoogleUserInfoProfile,
       GaiaConstants::kSecureConnectOAuth2Scope});
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::OnGetTokenSuccess(
    const TokenResponse& token_response) {
  access_token_ = token_response.access_token;
  FetchUserInfo();
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::OnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  // TODO(b/223628330): Implement retry strategy.
  LOG(ERROR) << "Failed to fetch access token for consumer: "
             << GetConsumerName() << " with error: " << error.ToString();
  std::move(callback_).Run(/*status=*/Status::kGetTokenError,
                           /*policy=*/absl::nullopt,
                           /*domain=*/std::string());
}

std::string UserCloudSigninRestrictionPolicyFetcherChromeOS::GetConsumerName()
    const {
  return "signin_restriction_policy_fetcher";
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::FetchUserInfo() {
  user_info_fetcher_->Start(access_token_);
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::OnGetUserInfoSuccess(
    const base::Value::Dict& user_info) {
  // Check if the user account has a hosted domain.
  const std::string* hosted_domain = user_info.FindString(kHostedDomainKey);
  if (hosted_domain) {
    hosted_domain_ = *hosted_domain;
    GetSecondaryGoogleAccountUsageInternal();
  } else {
    // Non Enterprise accounts do not have restrictions.
    DVLOG(1) << "User account is not an Enterprise account";
    std::move(callback_).Run(/*status=*/Status::kUnsupportedAccountTypeError,
                             /*policy=*/absl::nullopt,
                             /*domain=*/std::string());
  }
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::OnGetUserInfoFailure(
    const GoogleServiceAuthError& error) {
  LOG(ERROR) << "Failed to fetch user info: " << error.ToString();
  std::move(callback_).Run(/*status=*/Status::kGetUserInfoError,
                           /*policy=*/absl::nullopt,
                           /*domain=*/std::string());
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::
    GetSecondaryGoogleAccountUsageInternal() {
  policy_fetch_start_time_ = base::TimeTicks::Now();
  // Each url loader can only be used for one request.
  DCHECK(!url_loader_);
  url_loader_ =
      CreateUrlLoader(GURL(GetSecureConnectApiGetAccountSigninRestrictionUrl()),
                      access_token_, kAnnotation);
  // base::Unretained is safe here because `url_loader_` is owned by `this`.
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&UserCloudSigninRestrictionPolicyFetcherChromeOS::
                         OnSecondaryGoogleAccountUsageResult,
                     base::Unretained(this)),
      1024 * 1024 /* 1 MiB */);
}

void UserCloudSigninRestrictionPolicyFetcherChromeOS::
    OnSecondaryGoogleAccountUsageResult(
        std::unique_ptr<std::string> response_body) {
  base::UmaHistogramMediumTimes(
      kSecondaryGoogleAccountUsageLatencyHistogramName,
      base::TimeTicks::Now() - policy_fetch_start_time_);
  absl::optional<std::string> restriction;
  Status status = Status::kUnknownError;
  std::unique_ptr<network::SimpleURLLoader> url_loader = std::move(url_loader_);

  GoogleServiceAuthError error = GoogleServiceAuthError::AuthErrorNone();
  absl::optional<int> response_code;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();

  // Check for network or HTTP errors.
  if (url_loader->NetError() != net::OK || !response_body) {
    if (response_code) {
      LOG(ERROR) << "SecondaryGoogleAccountUsage request "
                    "failed with HTTP code: "
                 << response_code.value();
      status = Status::kHttpError;
    } else {
      error =
          GoogleServiceAuthError::FromConnectionError(url_loader->NetError());
      LOG(ERROR) << "SecondaryGoogleAccountUsage request "
                    "failed with error: "
                 << url_loader->NetError();
      status = Status::kNetworkError;
    }
    std::move(callback_).Run(status, restriction, hosted_domain_);
    return;
  }

  if (error.state() == GoogleServiceAuthError::NONE) {
    auto result = base::JSONReader::Read(*response_body, base::JSON_PARSE_RFC);
    const std::string* policy_value =
        result ? result->FindStringKey("policyValue") : nullptr;

    if (policy_value) {
      restriction = *policy_value;
      status = Status::kSuccess;
    } else {
      LOG(ERROR) << "Failed to parse SecondaryGoogleAccountUsage response";
      status = Status::kParsingResponseError;
    }
  }

  std::move(callback_).Run(status, restriction, hosted_domain_);
}

std::string UserCloudSigninRestrictionPolicyFetcherChromeOS::
    GetSecureConnectApiGetAccountSigninRestrictionUrl() const {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(policy::switches::kSecureConnectApiUrl)) {
    return command_line->GetSwitchValueASCII(
        policy::switches::kSecureConnectApiUrl);
  }

  return kSecureConnectApiGetSecondaryGoogleAccountUsageUrl;
}

}  // namespace ash
