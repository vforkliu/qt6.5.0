// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/credit_card_cvc_authenticator.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autocomplete_history_manager.h"
#include "components/autofill/core/browser/autofill_download_manager.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/data_model/autofill_profile.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/metrics/form_events/form_events.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/payments/test_authentication_requester.h"
#include "components/autofill/core/browser/payments/test_payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_autofill_clock.h"
#include "components/autofill/core/browser/test_autofill_driver.h"
#include "components/autofill/core/browser/test_personal_data_manager.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/prefs/pref_service.h"
#include "components/security_state/core/security_state.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/test/test_sync_service.h"
#include "components/variations/scoped_variations_ids_provider.h"
#include "components/version_info/channel.h"
#include "net/base/url_util.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace autofill {
namespace {

const char kTestGUID[] = "00000000-0000-0000-0000-000000000001";
const char kTestNumber[] = "4234567890123456";  // Visa
const char16_t kTestNumber16[] = u"4234567890123456";

}  // namespace

class CreditCardCVCAuthenticatorTest : public testing::Test {
 public:
  CreditCardCVCAuthenticatorTest() {}

  void SetUp() override {
    autofill_client_.SetPrefs(test::PrefServiceForTesting());
    personal_data_manager_.Init(/*profile_database=*/database_,
                                /*account_database=*/nullptr,
                                /*pref_service=*/autofill_client_.GetPrefs(),
                                /*local_state=*/autofill_client_.GetPrefs(),
                                /*identity_manager=*/nullptr,
                                /*history_service=*/nullptr,
                                /*strike_database=*/nullptr,
                                /*image_fetcher=*/nullptr,
                                /*is_off_the_record=*/false);
    personal_data_manager_.SetPrefService(autofill_client_.GetPrefs());

    requester_ = std::make_unique<TestAuthenticationRequester>();
    autofill_driver_ =
        std::make_unique<testing::NiceMock<TestAutofillDriver>>();

    payments::TestPaymentsClient* payments_client =
        new payments::TestPaymentsClient(
            autofill_driver_->GetURLLoaderFactory(),
            autofill_client_.GetIdentityManager(), &personal_data_manager_);
    autofill_client_.set_test_payments_client(
        std::unique_ptr<payments::TestPaymentsClient>(payments_client));
    cvc_authenticator_ =
        std::make_unique<CreditCardCVCAuthenticator>(&autofill_client_);
  }

  void TearDown() override {
    // Order of destruction is important as AutofillDriver relies on
    // PersonalDataManager to be around when it gets destroyed.
    autofill_driver_.reset();

    personal_data_manager_.SetPrefService(nullptr);
    personal_data_manager_.ClearCreditCards();
  }

  CreditCard CreateServerCard(std::string guid, std::string number) {
    CreditCard masked_server_card = CreditCard();
    test::SetCreditCardInfo(&masked_server_card, "Elvis Presley",
                            number.c_str(), test::NextMonth().c_str(),
                            test::NextYear().c_str(), "1");
    masked_server_card.set_guid(guid);
    masked_server_card.set_record_type(CreditCard::MASKED_SERVER_CARD);

    personal_data_manager_.ClearCreditCards();
    personal_data_manager_.AddServerCreditCard(masked_server_card);

    return masked_server_card;
  }

  void OnDidGetRealPan(AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan,
                       bool is_virtual_card = false) {
    payments::FullCardRequest* full_card_request = GetFullCardRequest();
    DCHECK(full_card_request);

    // Mock user response.
    payments::FullCardRequest::UserProvidedUnmaskDetails details;
    details.cvc = u"123";
    full_card_request->OnUnmaskPromptAccepted(details);

    // Mock payments response.
    payments::PaymentsClient::UnmaskResponseDetails response;
    response.card_type = is_virtual_card
                             ? AutofillClient::PaymentsRpcCardType::kVirtualCard
                             : AutofillClient::PaymentsRpcCardType::kServerCard;
    full_card_request->OnDidGetRealPan(result,
                                       response.with_real_pan(real_pan));
  }

  payments::FullCardRequest* GetFullCardRequest() {
    return cvc_authenticator_->full_card_request_.get();
  }

 protected:
  std::unique_ptr<TestAuthenticationRequester> requester_;
  base::test::TaskEnvironment task_environment_;
  variations::ScopedVariationsIdsProvider scoped_variations_ids_provider_{
      variations::VariationsIdsProvider::Mode::kUseSignedInState};
  TestAutofillClient autofill_client_;
  std::unique_ptr<TestAutofillDriver> autofill_driver_;
  scoped_refptr<AutofillWebDataService> database_;
  TestPersonalDataManager personal_data_manager_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<CreditCardCVCAuthenticator> cvc_authenticator_;
};

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticateServerCardSuccess) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);

  cvc_authenticator_->Authenticate(&card, requester_->GetWeakPtr(),
                                   &personal_data_manager_);

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kSuccess, kTestNumber);
  EXPECT_TRUE((*requester_->did_succeed()));
  EXPECT_EQ(kTestNumber16, requester_->number());
}

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticateVirtualCardSuccess) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);
  card.set_record_type(CreditCard::RecordType::VIRTUAL_CARD);
  autofill_client_.set_last_committed_primary_main_frame_url(
      GURL("https://vcncvcretrievaltest.com/"));

  cvc_authenticator_->Authenticate(
      &card, requester_->GetWeakPtr(), &personal_data_manager_,
      "test_vcn_context_token",
      CardUnmaskChallengeOption{.id = "test_challenge_option_id",
                                .type = CardUnmaskChallengeOptionType::kCvc,
                                .challenge_input_length = 3U,
                                .cvc_position = CvcPosition::kBackOfCard});

  payments::FullCardRequest* full_card_request = GetFullCardRequest();
  ASSERT_TRUE(full_card_request->GetShouldUnmaskCardForTesting());
  absl::optional<CardUnmaskChallengeOption> challenge_option =
      full_card_request->GetUnmaskRequestDetailsForTesting()
          ->selected_challenge_option;
  ASSERT_TRUE(challenge_option);
  EXPECT_EQ(challenge_option->id, "test_challenge_option_id");
  EXPECT_EQ(challenge_option->type, CardUnmaskChallengeOptionType::kCvc);
  EXPECT_EQ(challenge_option->challenge_input_length, 3U);
  EXPECT_EQ(challenge_option->cvc_position, CvcPosition::kBackOfCard);

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kSuccess, kTestNumber);
  EXPECT_TRUE((*requester_->did_succeed()));
  EXPECT_EQ(kTestNumber16, requester_->number());
}

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticateVirtualCard_InvalidURL) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);
  card.set_record_type(CreditCard::RecordType::VIRTUAL_CARD);
  autofill_client_.set_last_committed_primary_main_frame_url(GURL());

  cvc_authenticator_->Authenticate(
      &card, requester_->GetWeakPtr(), &personal_data_manager_,
      "test_vcn_context_token",
      CardUnmaskChallengeOption{.id = "test_challenge_option_id",
                                .type = CardUnmaskChallengeOptionType::kCvc,
                                .challenge_input_length = 3U,
                                .cvc_position = CvcPosition::kBackOfCard});

  ASSERT_FALSE(GetFullCardRequest()->GetShouldUnmaskCardForTesting());
  EXPECT_FALSE(*requester_->did_succeed());
}

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticateNetworkError) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);

  cvc_authenticator_->Authenticate(&card, requester_->GetWeakPtr(),
                                   &personal_data_manager_);

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kNetworkError,
                  std::string());
  EXPECT_FALSE((*requester_->did_succeed()));
}

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticatePermanentFailure) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);

  cvc_authenticator_->Authenticate(&card, requester_->GetWeakPtr(),
                                   &personal_data_manager_);

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kPermanentFailure,
                  std::string());
  EXPECT_FALSE((*requester_->did_succeed()));
}

TEST_F(CreditCardCVCAuthenticatorTest, AuthenticateTryAgainFailure) {
  CreditCard card = CreateServerCard(kTestGUID, kTestNumber);

  cvc_authenticator_->Authenticate(&card, requester_->GetWeakPtr(),
                                   &personal_data_manager_);

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kTryAgainFailure,
                  std::string());
  EXPECT_FALSE(requester_->did_succeed().has_value());

  OnDidGetRealPan(AutofillClient::PaymentsRpcResult::kSuccess, kTestNumber);
  EXPECT_TRUE((*requester_->did_succeed()));
  EXPECT_EQ(kTestNumber16, requester_->number());
}

}  // namespace autofill
