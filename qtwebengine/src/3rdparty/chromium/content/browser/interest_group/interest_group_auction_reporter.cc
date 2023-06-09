// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/interest_group/interest_group_auction_reporter.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "base/trace_event/common/trace_event_common.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/fenced_frame/fenced_frame_url_mapping.h"
#include "content/browser/interest_group/auction_worklet_manager.h"
#include "content/browser/interest_group/interest_group_auction.h"
#include "content/browser/interest_group/interest_group_storage.h"
#include "content/services/auction_worklet/public/mojom/bidder_worklet.mojom.h"
#include "content/services/auction_worklet/public/mojom/private_aggregation_request.mojom-forward.h"
#include "content/services/auction_worklet/public/mojom/private_aggregation_request.mojom.h"
#include "content/services/auction_worklet/public/mojom/seller_worklet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/interest_group/interest_group.h"
#include "third_party/blink/public/mojom/fenced_frame/fenced_frame.mojom-shared.h"
#include "third_party/blink/public/mojom/interest_group/interest_group_types.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace {

// All event-level reporting URLs received from worklets must be valid HTTPS
// URLs. It's up to callers to call ReportBadMessage() on invalid URLs.
bool IsEventLevelReportingUrlValid(const GURL& url) {
  return url.is_valid() && url.SchemeIs(url::kHttpsScheme);
}

}  // namespace

InterestGroupAuctionReporter::SellerWinningBidInfo::SellerWinningBidInfo() =
    default;
InterestGroupAuctionReporter::SellerWinningBidInfo::SellerWinningBidInfo(
    SellerWinningBidInfo&&) = default;
InterestGroupAuctionReporter::SellerWinningBidInfo::~SellerWinningBidInfo() =
    default;

InterestGroupAuctionReporter::WinningBidInfo::WinningBidInfo() = default;
InterestGroupAuctionReporter::WinningBidInfo::WinningBidInfo(WinningBidInfo&&) =
    default;
InterestGroupAuctionReporter::WinningBidInfo::~WinningBidInfo() = default;

InterestGroupAuctionReporter::InterestGroupAuctionReporter(
    AuctionWorkletManager* auction_worklet_manager,
    WinningBidInfo winning_bid_info,
    SellerWinningBidInfo top_level_seller_winning_bid_info,
    absl::optional<SellerWinningBidInfo> component_seller_winning_bid_info,
    std::map<url::Origin, PrivateAggregationRequests>
        private_aggregation_requests)
    : auction_worklet_manager_(auction_worklet_manager),
      winning_bid_info_(std::move(winning_bid_info)),
      top_level_seller_winning_bid_info_(
          std::move(top_level_seller_winning_bid_info)),
      component_seller_winning_bid_info_(
          std::move(component_seller_winning_bid_info)),
      private_aggregation_requests_(std::move(private_aggregation_requests)) {}

InterestGroupAuctionReporter ::~InterestGroupAuctionReporter() = default;

void InterestGroupAuctionReporter::Start(base::OnceClosure callback) {
  DCHECK(!callback_);

  callback_ = std::move(callback);
  RequestSellerWorklet(&top_level_seller_winning_bid_info_,
                       /*top_seller_signals=*/absl::nullopt);
}

void InterestGroupAuctionReporter::RequestSellerWorklet(
    const SellerWinningBidInfo* seller_info,
    const absl::optional<std::string>& top_seller_signals) {
  seller_worklet_handle_.reset();
  // base::Unretained is safe to use for these callbacks because destroying
  // `seller_worklet_handle_` will prevent the callbacks from being invoked, if
  // `this` is destroyed while still waiting on the callbacks.
  if (auction_worklet_manager_->RequestSellerWorklet(
          seller_info->auction_config->decision_logic_url,
          seller_info->auction_config->trusted_scoring_signals_url,
          seller_info->auction_config->seller_experiment_group_id,
          base::BindOnce(&InterestGroupAuctionReporter::OnSellerWorkletReceived,
                         base::Unretained(this), base::Unretained(seller_info),
                         top_seller_signals),
          base::BindOnce(
              &InterestGroupAuctionReporter::OnSellerWorkletFatalError,
              base::Unretained(this), base::Unretained(seller_info)),
          seller_worklet_handle_)) {
    OnSellerWorkletReceived(seller_info, top_seller_signals);
  }
}

void InterestGroupAuctionReporter::OnSellerWorkletFatalError(
    const SellerWinningBidInfo* seller_info,
    AuctionWorkletManager::FatalErrorType fatal_error_type,
    const std::vector<std::string>& errors) {
  // On a seller load failure or crash, act as if the worklet returned no
  // results to advance to the next worklet.
  OnSellerReportResultComplete(seller_info,
                               /*signals_for_winner=*/absl::nullopt,
                               /*seller_report_url=*/absl::nullopt,
                               /*seller_ad_beacon_map=*/{},
                               /*pa_requests=*/{}, errors);
}

void InterestGroupAuctionReporter::OnSellerWorkletReceived(
    const SellerWinningBidInfo* seller_info,
    const absl::optional<std::string>& top_seller_signals) {
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("fledge", "seller_worklet_report_result",
                                    seller_info->trace_id);

  auction_worklet::mojom::ComponentAuctionOtherSellerPtr other_seller;
  auction_worklet::mojom::ComponentAuctionReportResultParamsPtr
      browser_signals_component_auction_report_result_params;
  if (seller_info == &top_level_seller_winning_bid_info_) {
    DCHECK(!top_seller_signals);
    if (component_seller_winning_bid_info_) {
      other_seller = auction_worklet::mojom::ComponentAuctionOtherSeller::
          NewComponentSeller(
              component_seller_winning_bid_info_->auction_config->seller);
    }
  } else {
    DCHECK(top_seller_signals);
    DCHECK(component_seller_winning_bid_info_);
    DCHECK(seller_info->component_auction_modified_bid_params);
    other_seller =
        auction_worklet::mojom::ComponentAuctionOtherSeller::NewTopLevelSeller(
            top_level_seller_winning_bid_info_.auction_config->seller);
    browser_signals_component_auction_report_result_params =
        auction_worklet::mojom::ComponentAuctionReportResultParams::New(
            /*top_level_seller_signals=*/std::move(top_seller_signals).value(),
            /*modified_bid=*/
            seller_info->component_auction_modified_bid_params->bid,
            /*has_modified_bid=*/
            seller_info->component_auction_modified_bid_params->has_bid);
  }

  seller_worklet_handle_->GetSellerWorklet()->ReportResult(
      seller_info->auction_config->non_shared_params, std::move(other_seller),
      winning_bid_info_.storage_interest_group->interest_group.owner,
      winning_bid_info_.render_url, seller_info->bid, seller_info->score,
      seller_info->highest_scoring_other_bid,
      std::move(browser_signals_component_auction_report_result_params),
      seller_info->scoring_signals_data_version.value_or(0),
      seller_info->scoring_signals_data_version.has_value(),
      seller_info->trace_id,
      base::BindOnce(
          &InterestGroupAuctionReporter::OnSellerReportResultComplete,
          weak_ptr_factory_.GetWeakPtr(), base::Unretained(seller_info)));
}

void InterestGroupAuctionReporter::OnSellerReportResultComplete(
    const SellerWinningBidInfo* seller_info,
    const absl::optional<std::string>& signals_for_winner,
    const absl::optional<GURL>& seller_report_url,
    const base::flat_map<std::string, GURL>& seller_ad_beacon_map,
    PrivateAggregationRequests pa_requests,
    const std::vector<std::string>& errors) {
  TRACE_EVENT_NESTABLE_ASYNC_END0("fledge", "seller_worklet_report_result",
                                  seller_info->trace_id);
  seller_worklet_handle_.reset();

  // The mojom API declaration should ensure none of these are null.
  DCHECK(base::ranges::none_of(
      pa_requests,
      [](const auction_worklet::mojom::PrivateAggregationRequestPtr&
             request_ptr) { return request_ptr.is_null(); }));
  if (!pa_requests.empty()) {
    PrivateAggregationRequests& pa_requests_for_seller =
        private_aggregation_requests_[seller_info->auction_config->seller];
    pa_requests_for_seller.insert(pa_requests_for_seller.end(),
                                  std::move_iterator(pa_requests.begin()),
                                  std::move_iterator(pa_requests.end()));
  }

  if (!seller_ad_beacon_map.empty()) {
    for (const auto& element : seller_ad_beacon_map) {
      if (!IsEventLevelReportingUrlValid(element.second)) {
        mojo::ReportBadMessage(base::StrCat(
            {"Invalid seller beacon URL for '", element.first, "'"}));
        // TODO(mmenke): Call into other worklets, despite failure.
        OnReportingComplete(
            /*success=*/false);
        return;
      }
    }
    if (seller_info == &top_level_seller_winning_bid_info_) {
      ad_beacon_map_.metadata[blink::mojom::ReportingDestination::kSeller] =
          seller_ad_beacon_map;
    } else {
      ad_beacon_map_
          .metadata[blink::mojom::ReportingDestination::kComponentSeller] =
          seller_ad_beacon_map;
    }
  }

  if (seller_report_url) {
    if (!IsEventLevelReportingUrlValid(*seller_report_url)) {
      mojo::ReportBadMessage("Invalid seller report URL");
      // TODO(mmenke): Call into other worklets, despite failure.
      OnReportingComplete(/*success=*/false);
      return;
    }

    report_urls_.push_back(*seller_report_url);
  }

  errors_.insert(errors_.end(), errors.begin(), errors.end());

  // Treat a null `signals_for_winner` value as a null JS response.
  //
  // TODO(mmenke): Consider making `signals_for_winner` itself non-optional, and
  // clean this up.
  std::string fixed_up_signals_for_winner = signals_for_winner.value_or("null");

  // If a the winning bid is from a nested component auction, need to call into
  // that Auction's report logic (which will invoke both that seller's
  // ReportResult() method, and the bidder's ReportWin()).
  if (seller_info == &top_level_seller_winning_bid_info_ &&
      component_seller_winning_bid_info_) {
    RequestSellerWorklet(&component_seller_winning_bid_info_.value(),
                         fixed_up_signals_for_winner);
    return;
  }

  RequestBidderWorklet(fixed_up_signals_for_winner);
}

void InterestGroupAuctionReporter::RequestBidderWorklet(
    const std::string& signals_for_winner) {
  // Seller worklets should have been unloaded by now, and bidder worklet should
  // not have been loaded yet.
  DCHECK(!seller_worklet_handle_);
  DCHECK(!bidder_worklet_handle_);

  const blink::InterestGroup& interest_group =
      winning_bid_info_.storage_interest_group->interest_group;

  const blink::AuctionConfig* auction_config =
      GetBidderAuction().auction_config;
  absl::optional<uint16_t> experiment_group_id =
      InterestGroupAuction::GetBuyerExperimentId(*auction_config,
                                                 interest_group.owner);

  // base::Unretained is safe to use for these callbacks because destroying
  // `bidder_worklet_handle_` will prevent the callbacks from being invoked, if
  // `this` is destroyed while still waiting on the callbacks.
  if (auction_worklet_manager_->RequestBidderWorklet(
          interest_group.bidding_url.value_or(GURL()),
          interest_group.bidding_wasm_helper_url,
          interest_group.trusted_bidding_signals_url, experiment_group_id,
          base::BindOnce(&InterestGroupAuctionReporter::OnBidderWorkletReceived,
                         base::Unretained(this), signals_for_winner),
          base::BindOnce(
              &InterestGroupAuctionReporter::OnBidderWorkletFatalError,
              base::Unretained(this)),
          bidder_worklet_handle_)) {
    OnBidderWorkletReceived(signals_for_winner);
  }
}

void InterestGroupAuctionReporter::OnBidderWorkletReceived(
    const std::string& signals_for_winner) {
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      "fledge", "bidder_worklet_report_win",
      top_level_seller_winning_bid_info_.trace_id);

  const SellerWinningBidInfo& seller_info = GetBidderAuction();
  const blink::AuctionConfig* auction_config = seller_info.auction_config;
  absl::optional<std::string> per_buyer_signals =
      InterestGroupAuction::GetPerBuyerSignals(
          *auction_config,
          winning_bid_info_.storage_interest_group->interest_group.owner);

  bidder_worklet_handle_->GetBidderWorklet()->ReportWin(
      winning_bid_info_.storage_interest_group->interest_group.name,
      auction_config->non_shared_params.auction_signals, per_buyer_signals,
      signals_for_winner, winning_bid_info_.render_url, winning_bid_info_.bid,
      /*browser_signal_highest_scoring_other_bid=*/
      seller_info.highest_scoring_other_bid,
      seller_info.highest_scoring_other_bid_owner.has_value() &&
          winning_bid_info_.storage_interest_group->interest_group.owner ==
              seller_info.highest_scoring_other_bid_owner.value(),
      auction_config->seller,
      /*browser_signal_top_level_seller_origin=*/
      component_seller_winning_bid_info_
          ? top_level_seller_winning_bid_info_.auction_config->seller
          : absl::optional<url::Origin>(),
      winning_bid_info_.bidding_signals_data_version.value_or(0),
      winning_bid_info_.bidding_signals_data_version.has_value(),
      top_level_seller_winning_bid_info_.trace_id,
      base::BindOnce(&InterestGroupAuctionReporter::OnBidderReportWinComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void InterestGroupAuctionReporter::OnBidderWorkletFatalError(
    AuctionWorkletManager::FatalErrorType fatal_error_type,
    const std::vector<std::string>& errors) {
  // Nothing more to do. Act as if the worklet completed as normal, with no
  // results.
  OnBidderReportWinComplete(/*bidder_report_url=*/absl::nullopt,
                            /*bidder_ad_beacon_map=*/{},
                            /*pa_requests=*/{}, errors);
}

void InterestGroupAuctionReporter::OnBidderReportWinComplete(
    const absl::optional<GURL>& bidder_report_url,
    const base::flat_map<std::string, GURL>& bidder_ad_beacon_map,
    PrivateAggregationRequests pa_requests,
    const std::vector<std::string>& errors) {
  TRACE_EVENT_NESTABLE_ASYNC_END0("fledge", "bidder_worklet_report_win",
                                  top_level_seller_winning_bid_info_.trace_id);

  bidder_worklet_handle_.reset();

  // There should be at most two other report URL at this point.
  DCHECK_LE(report_urls_.size(), 2u);

  // The mojom API declaration should ensure none of these are null.
  DCHECK(base::ranges::none_of(
      pa_requests,
      [](const auction_worklet::mojom::PrivateAggregationRequestPtr&
             request_ptr) { return request_ptr.is_null(); }));
  if (!pa_requests.empty()) {
    PrivateAggregationRequests& pa_requests_for_bidder =
        private_aggregation_requests_[winning_bid_info_.storage_interest_group
                                          ->interest_group.owner];
    pa_requests_for_bidder.insert(pa_requests_for_bidder.end(),
                                  std::move_iterator(pa_requests.begin()),
                                  std::move_iterator(pa_requests.end()));
  }

  if (!bidder_ad_beacon_map.empty()) {
    for (const auto& element : bidder_ad_beacon_map) {
      if (!IsEventLevelReportingUrlValid(element.second)) {
        mojo::ReportBadMessage(base::StrCat(
            {"Invalid bidder beacon URL for '", element.first, "'"}));
        OnReportingComplete(
            /*success=*/false);
        return;
      }
    }
    ad_beacon_map_.metadata[blink::mojom::ReportingDestination::kBuyer] =
        bidder_ad_beacon_map;
  }

  if (bidder_report_url) {
    if (!IsEventLevelReportingUrlValid(*bidder_report_url)) {
      mojo::ReportBadMessage("Invalid bidder report URL");
      OnReportingComplete(/*success=*/false);
      return;
    }

    report_urls_.push_back(*bidder_report_url);
  }

  OnReportingComplete(/*success=*/true, errors);
}

void InterestGroupAuctionReporter::OnReportingComplete(
    bool success,
    const std::vector<std::string>& errors) {
  if (!success) {
    private_aggregation_requests_.clear();
    ad_beacon_map_ = ReportingMetadata();
    report_urls_.clear();
  }
  errors_.insert(errors_.end(), errors.begin(), errors.end());
  std::move(callback_).Run();
}

const InterestGroupAuctionReporter::SellerWinningBidInfo&
InterestGroupAuctionReporter::GetBidderAuction() {
  if (component_seller_winning_bid_info_)
    return component_seller_winning_bid_info_.value();
  return top_level_seller_winning_bid_info_;
}

}  // namespace content
