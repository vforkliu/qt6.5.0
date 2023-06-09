// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reporting/metrics/fake_sampler.h"

#include <utility>

#include "base/check.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace reporting {
namespace test {

FakeSampler::FakeSampler() = default;

FakeSampler::~FakeSampler() = default;

void FakeSampler::MaybeCollect(OptionalMetricCallback cb) {
  ++num_calls_;
  std::move(cb).Run(metric_data_);
}

void FakeSampler::SetMetricData(absl::optional<MetricData> metric_data) {
  metric_data_ = std::move(metric_data);
}

int FakeSampler::GetNumCollectCalls() const {
  return num_calls_;
}

FakeDelayedSampler::FakeDelayedSampler() = default;

FakeDelayedSampler::~FakeDelayedSampler() = default;

void FakeDelayedSampler::MaybeCollect(OptionalMetricCallback cb) {
  ++num_calls_;
  cb_ = std::move(cb);
}

void FakeDelayedSampler::RunCallback() {
  DCHECK(cb_);
  std::move(cb_).Run(metric_data_);
}

FakeMetricEventObserver::FakeMetricEventObserver() = default;

FakeMetricEventObserver::~FakeMetricEventObserver() = default;

void FakeMetricEventObserver::SetOnEventObservedCallback(
    MetricRepeatingCallback cb) {
  EXPECT_FALSE(cb_);
  cb_ = std::move(cb);
}

void FakeMetricEventObserver::SetReportingEnabled(bool is_enabled) {
  is_reporting_enabled_ = is_enabled;
}

void FakeMetricEventObserver::RunCallback(MetricData metric_data) {
  base::RunLoop run_loop;
  cb_.Run(std::move(metric_data));
  base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                   run_loop.QuitClosure());
  run_loop.Run();
}

bool FakeMetricEventObserver::GetReportingEnabled() const {
  return is_reporting_enabled_;
}
}  // namespace test
}  // namespace reporting
