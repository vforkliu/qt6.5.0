// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_TEST_WAYLAND_DRAG_DROP_TEST_H_
#define UI_OZONE_PLATFORM_WAYLAND_TEST_WAYLAND_DRAG_DROP_TEST_H_

#include <cstdint>

#include "base/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/dragdrop/os_exchange_data_provider_factory_ozone.h"
#include "ui/ozone/platform/wayland/test/test_data_device.h"
#include "ui/ozone/platform/wayland/test/test_data_source.h"
#include "ui/ozone/platform/wayland/test/wayland_test.h"

namespace gfx {
class Point;
}

namespace wl {
class MockSurface;
class TestDataDeviceManager;
}  // namespace wl

namespace ui {

class WaylandWindow;

class TestWaylandOSExchangeDataProvideFactory
    : public OSExchangeDataProviderFactoryOzone {
 public:
  TestWaylandOSExchangeDataProvideFactory();
  ~TestWaylandOSExchangeDataProvideFactory() override;

  std::unique_ptr<OSExchangeDataProvider> CreateProvider() override;
};

// Base class for Wayland drag-and-drop tests. Public methods allow test code to
// emulate dnd-related events from the test compositor and can be used in both
// data and window dragging test cases.
class WaylandDragDropTest : public WaylandTest,
                            public wl::TestDataDevice::DragDelegate {
 public:
  WaylandDragDropTest();
  WaylandDragDropTest(const WaylandDragDropTest&) = delete;
  WaylandDragDropTest& operator=(const WaylandDragDropTest&) = delete;
  ~WaylandDragDropTest() override;

  // These are public for convenience, as they must be callable from lambda
  // functions, usually posted to task queue while the drag loop runs.
  void SendDndEnter(WaylandWindow* window, const gfx::Point& location);
  void SendDndLeave();
  void SendDndMotion(const gfx::Point& location);
  void SendDndDrop();
  void SendDndCancelled();
  void SendDndAction(uint32_t action);
  void ReadData(const std::string& mime_type,
                wl::TestDataSource::ReadDataCallback callback);

  virtual void SendPointerEnter(WaylandWindow* window,
                                MockPlatformWindowDelegate* delegate);
  virtual void SendPointerLeave(WaylandWindow* window,
                                MockPlatformWindowDelegate* delegate);
  virtual void SendPointerButton(WaylandWindow* window,
                                 MockPlatformWindowDelegate* delegate,
                                 int button,
                                 bool pressed);

  virtual void SendTouchDown(WaylandWindow* window,
                             MockPlatformWindowDelegate* delegate,
                             int id,
                             const gfx::Point& location);
  virtual void SendTouchUp(int id);
  virtual void SendTouchMotion(WaylandWindow* window,
                               MockPlatformWindowDelegate* delegate,
                               int id,
                               const gfx::Point& location);

  MOCK_METHOD3(MockStartDrag,
               void(wl::TestDataSource* source,
                    wl::MockSurface* origin,
                    uint32_t serial));

 protected:
  // WaylandTest:
  void SetUp() override;
  void TearDown() override;

  // wl::TestDataDevice::DragDelegate:
  void StartDrag(wl::TestDataSource* source,
                 wl::MockSurface* origin,
                 uint32_t serial) override;

  void OfferAndEnter(wl::MockSurface* surface, const gfx::Point& location);
  uint32_t NextSerial();
  uint32_t NextTime() const;
  void ScheduleTestTask(base::OnceClosure test_task);
  void RunTestTask(base::OnceClosure test_task);

  WaylandWindowManager* window_manager() const {
    return connection_->wayland_window_manager();
  }

  // Server objects
  raw_ptr<wl::TestDataDeviceManager> data_device_manager_;
  raw_ptr<wl::TestDataSource> data_source_;
  raw_ptr<wl::MockPointer> pointer_;
  raw_ptr<wl::TestTouch> touch_;

  uint32_t current_serial_;

 private:
  TestWaylandOSExchangeDataProvideFactory os_exchange_factory_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_TEST_WAYLAND_DRAG_DROP_TEST_H_
