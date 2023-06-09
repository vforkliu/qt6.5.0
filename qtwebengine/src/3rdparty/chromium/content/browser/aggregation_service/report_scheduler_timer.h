// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_AGGREGATION_SERVICE_REPORT_SCHEDULER_TIMER_H_
#define CONTENT_BROWSER_AGGREGATION_SERVICE_REPORT_SCHEDULER_TIMER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "content/common/content_export.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
}  // namespace base

namespace content {

// This class consolidates logic regarding when to schedule the browser to send
// reports for APIs using the aggregation service and for event-level
// attribution reporting. This includes handling network changes and browser
// startup. It delegates certain operations to API-specific implementations.
// TODO(alexmt): Consider moving out of the aggregation_service directory to a
// separate shared directory.
class CONTENT_EXPORT ReportSchedulerTimer
    : public network::NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Should be overridden with a method that gets the next report time that
    // the timer should fire at and returns it via the callback. If there is no
    // next report time, `absl::nullopt` should be returned instead.
    virtual void GetNextReportTime(
        base::OnceCallback<void(absl::optional<base::Time>)>,
        base::Time now) = 0;

    // Called when the timer is fired, with the current time `now`. `Refresh()`
    // is automatically called after. If this causes a `GetNextReportTime()`
    // call, that will be passed the same `now`.
    virtual void OnReportingTimeReached(base::Time now) = 0;

    // Called when the connection changes from offline to online. May also be
    // called on a connection change if there are no stored reports, see
    // `OnConnectionChanged()`. Running the callback will call `MaybeSet()` with
    // the given argument; this may be necessary after the report times were
    // adjusted.
    virtual void AdjustOfflineReportTimes(
        base::OnceCallback<void(absl::optional<base::Time>)>) = 0;
  };

  explicit ReportSchedulerTimer(std::unique_ptr<Delegate> delegate);

  ReportSchedulerTimer(const ReportSchedulerTimer&) = delete;
  ReportSchedulerTimer& operator=(const ReportSchedulerTimer&) = delete;
  ReportSchedulerTimer(ReportSchedulerTimer&&) = delete;
  ReportSchedulerTimer& operator=(ReportSchedulerTimer&&) = delete;

  ~ReportSchedulerTimer() override;

  // Schedules `reporting_time_reached_timer_` to fire at that time, unless the
  // timer is already set to fire earlier.
  void MaybeSet(absl::optional<base::Time> reporting_time);

  // Updates the timer based on the returned value from
  // `Delegate::GetNextReportTime()`.
  void Refresh();

 private:
  void OnTimerFired();
  void RefreshImpl(base::Time now);

  // network::NetworkConnectionTracker::NetworkConnectionObserver:
  void OnConnectionChanged(network::mojom::ConnectionType) override;

  // Fires whenever a reporting time is reached for a report. Must be updated
  // whenever the next report time changes.
  base::WallClockTimer reporting_time_reached_timer_;

  const std::unique_ptr<Delegate> delegate_;

  base::WeakPtrFactory<ReportSchedulerTimer> weak_ptr_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_AGGREGATION_SERVICE_REPORT_SCHEDULER_TIMER_H_
