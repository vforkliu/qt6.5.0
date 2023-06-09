// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GEOLOCATION_FAKE_LOCATION_PROVIDER_H_
#define SERVICES_DEVICE_GEOLOCATION_FAKE_LOCATION_PROVIDER_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

// Fake implementation of a location provider for testing.
class FakeLocationProvider : public LocationProvider {
 public:
  enum State { STOPPED, LOW_ACCURACY, HIGH_ACCURACY } state_ = STOPPED;

  FakeLocationProvider();

  FakeLocationProvider(const FakeLocationProvider&) = delete;
  FakeLocationProvider& operator=(const FakeLocationProvider&) = delete;

  ~FakeLocationProvider() override;

  // Updates listeners with the new position.
  void HandlePositionChanged(const mojom::Geoposition& position);

  State state() const { return state_; }
  bool is_permission_granted() const { return is_permission_granted_; }

  // LocationProvider implementation.
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const mojom::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

  scoped_refptr<base::SingleThreadTaskRunner> provider_task_runner_;

 private:
  bool is_permission_granted_ = false;
  mojom::Geoposition position_;
  LocationProviderUpdateCallback callback_;
};

}  // namespace device

#endif  // SERVICES_DEVICE_GEOLOCATION_FAKE_LOCATION_PROVIDER_H_
