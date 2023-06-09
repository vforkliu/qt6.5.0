// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/input.h>
#include <wayland-server.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/containers/flat_set.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "ui/base/clipboard/clipboard_constants.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/geometry/point.h"
#include "ui/ozone/platform/wayland/host/wayland_clipboard.h"
#include "ui/ozone/platform/wayland/host/wayland_connection_test_api.h"
#include "ui/ozone/platform/wayland/host/wayland_serial_tracker.h"
#include "ui/ozone/platform/wayland/test/mock_pointer.h"
#include "ui/ozone/platform/wayland/test/mock_surface.h"
#include "ui/ozone/platform/wayland/test/test_data_device.h"
#include "ui/ozone/platform/wayland/test/test_data_device_manager.h"
#include "ui/ozone/platform/wayland/test/test_data_offer.h"
#include "ui/ozone/platform/wayland/test/test_data_source.h"
#include "ui/ozone/platform/wayland/test/test_keyboard.h"
#include "ui/ozone/platform/wayland/test/test_selection_device_manager.h"
#include "ui/ozone/platform/wayland/test/test_touch.h"
#include "ui/ozone/platform/wayland/test/test_wayland_server_thread.h"
#include "ui/ozone/platform/wayland/test/wayland_test.h"
#include "ui/ozone/public/platform_clipboard.h"

using testing::_;
using testing::Eq;
using testing::IsEmpty;
using testing::IsNull;
using testing::Mock;
using testing::Values;

namespace ui {

namespace {

constexpr char kSampleClipboardText[] = "This is a sample text for clipboard.";

template <typename StringType>
ui::PlatformClipboard::Data ToClipboardData(const StringType& data_string) {
  std::vector<uint8_t> data_vector;
  data_vector.assign(data_string.begin(), data_string.end());
  return base::RefCountedBytes::TakeVector(&data_vector);
}

}  // namespace

class WaylandClipboardTestBase : public WaylandTest {
 public:
  void SetUp() override {
    WaylandTest::SetUp();

    wl_seat_send_capabilities(server_.seat()->resource(),
                              WL_SEAT_CAPABILITY_POINTER |
                                  WL_SEAT_CAPABILITY_TOUCH |
                                  WL_SEAT_CAPABILITY_KEYBOARD);
    Sync();

    pointer_ = server_.seat()->pointer();
    ASSERT_TRUE(pointer_);

    touch_ = server_.seat()->touch();
    ASSERT_TRUE(touch_);

    keyboard_ = server_.seat()->keyboard();
    ASSERT_TRUE(keyboard_);

    // As of now, WaylandClipboard::RequestClipboardData is implemented in a
    // blocking way, which requires a roundtrip before attempting the data
    // from the selection fd. As Wayland events polling is single-threaded for
    // tests, WaylandConnection's roundtrip implementation must be hooked up
    // here to make sure that the required test compositor calls are done,
    // otherwise tests will enter in a dead lock.
    // TODO(crbug.com/443355): Remove once Clipboard API becomes async.
    WaylandConnectionTestApi(connection_.get())
        .SetRoundtripClosure(base::BindLambdaForTesting([&]() {
          wl_display_flush(connection_->display());
          Sync();
          base::ThreadPoolInstance::Get()->FlushForTesting();
        }));

    clipboard_ = connection_->clipboard();
    ASSERT_TRUE(clipboard_);
  }

  void TearDown() override {
    WaylandTest::TearDown();
    clipboard_ = nullptr;
  }

 protected:
  // Ensure the requests/events are flushed and posted tasks get processed.
  // Actual clipboard data reading is performed in the ThreadPool. Also,
  // wl::TestSelection{Source,Offer} currently use ThreadPool task runners.
  void WaitForClipboardTasks() {
    Sync();
    base::ThreadPoolInstance::Get()->FlushForTesting();
    base::RunLoop().RunUntilIdle();
  }

  void SentPointerButtonPress(const gfx::Point& location) {
    wl_pointer_send_enter(pointer_->resource(), ++serial_, surface_->resource(),
                          location.x(), location.y());
    wl_pointer_send_button(pointer_->resource(), ++serial_, ++timestamp_,
                           BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
  }
  void SendTouchDown(const gfx::Point& location) {
    wl_touch_send_down(touch_->resource(), ++serial_, ++timestamp_,
                       surface_->resource(), /*touch_id=*/0,
                       wl_fixed_from_int(location.x()),
                       wl_fixed_from_int(location.y()));
    wl_touch_send_frame(touch_->resource());
  }

  void SendTouchUp() {
    wl_touch_send_up(touch_->resource(), ++serial_, ++timestamp_,
                     /*touch_id=*/0);
  }

  void SendKeyboardKey() {
    struct wl_array empty;
    wl_array_init(&empty);
    wl_keyboard_send_enter(keyboard_->resource(), 1, surface_->resource(),
                           &empty);
    wl_array_release(&empty);
    wl_keyboard_send_key(keyboard_->resource(), 2, 0, 30 /* a */,
                         WL_KEYBOARD_KEY_STATE_PRESSED);
  }

  /* Server objects */
  raw_ptr<wl::MockPointer> pointer_;
  raw_ptr<wl::TestTouch> touch_;
  raw_ptr<wl::TestKeyboard> keyboard_;

  raw_ptr<WaylandClipboard> clipboard_ = nullptr;

  uint32_t serial_ = 0;
  uint32_t timestamp_ = 0;
};

class WaylandClipboardTest : public WaylandClipboardTestBase {
 public:
  WaylandClipboardTest() = default;

  WaylandClipboardTest(const WaylandClipboardTest&) = delete;
  WaylandClipboardTest& operator=(const WaylandClipboardTest&) = delete;

  ~WaylandClipboardTest() override = default;

  void SetUp() override {
    WaylandClipboardTestBase::SetUp();

    ASSERT_TRUE(server_.data_device_manager());
    ASSERT_TRUE(GetParam().primary_selection_protocol ==
                    wl::PrimarySelectionProtocol::kNone ||
                server_.primary_selection_device_manager());

    // Make sure clipboard instance for the available primary selection protocol
    // gets properly created, ie: the corresponding 'get_device' request is
    // issued, so server-side objects are created prior to test-case specific
    // calls, otherwise tests, such as ReadFromClipboard, would crash.
    ASSERT_EQ(GetBuffer() == ClipboardBuffer::kSelection,
              !!clipboard_->GetClipboard(ClipboardBuffer::kSelection));
    Sync();

    offered_data_.clear();
  }

 protected:
  wl::TestSelectionDevice* GetServerSelectionDevice() {
    return GetBuffer() == ClipboardBuffer::kSelection
               ? server_.primary_selection_device_manager()->device()
               : server_.data_device_manager()->data_device();
  }

  wl::TestSelectionSource* GetServerSelectionSource() {
    return GetBuffer() == ClipboardBuffer::kSelection
               ? server_.primary_selection_device_manager()->source()
               : server_.data_device_manager()->data_source();
  }

  ClipboardBuffer GetBuffer() const {
    return GetParam().primary_selection_protocol !=
                   wl::PrimarySelectionProtocol::kNone
               ? ClipboardBuffer::kSelection
               : ClipboardBuffer::kCopyPaste;
  }

  void ResetServerSelectionSource() {
    if (GetParam().primary_selection_protocol !=
        wl::PrimarySelectionProtocol::kNone) {
      server_.primary_selection_device_manager()->set_source(nullptr);
    } else {
      server_.data_device_manager()->set_data_source(nullptr);
    }
  }

  // Fill the clipboard backing store with sample data.
  void OfferData(ClipboardBuffer buffer,
                 const char* data,
                 const std::string& mime_type) {
    std::vector<uint8_t> data_vector(data, data + std::strlen(data));
    offered_data_[mime_type] = base::RefCountedBytes::TakeVector(&data_vector);

    base::MockCallback<PlatformClipboard::OfferDataClosure> offer_callback;
    EXPECT_CALL(offer_callback, Run()).Times(1);
    clipboard_->OfferClipboardData(buffer, offered_data_, offer_callback.Get());
  }

  PlatformClipboard::DataMap offered_data_;
};

class CopyPasteOnlyClipboardTest : public WaylandClipboardTestBase {
 public:
  void SetUp() override {
    WaylandClipboardTestBase::SetUp();

    ASSERT_FALSE(clipboard_->IsSelectionBufferAvailable());

    ASSERT_EQ(wl::PrimarySelectionProtocol::kNone,
              GetParam().primary_selection_protocol);
    ASSERT_TRUE(server_.data_device_manager());
    ASSERT_FALSE(server_.primary_selection_device_manager());
  }
};

// Verifies that copy-to-clipboard works as expected. Actual Wayland input
// events are used in order to exercise all the components involved, e.g:
// Wayland{Pointer,Keyboard,Touch}, Serial tracker and WaylandClipboard.
//
// Regression test for https://crbug.com/1282220.
TEST_P(WaylandClipboardTest, WriteToClipboard) {
  const base::RepeatingClosure send_input_event_closures[]{
      // Mouse button press
      base::BindLambdaForTesting([&]() {
        SentPointerButtonPress({10, 10});
      }),
      // Key press
      base::BindLambdaForTesting([&]() { SendKeyboardKey(); }),
      // Touch down
      base::BindLambdaForTesting([&]() {
        SendTouchDown({200, 200});
      }),
      // Touch tap (down > up)
      base::BindLambdaForTesting([&]() {
        SendTouchDown({300, 300});
        SendTouchUp();
      })};

  auto* window_manager = connection_->wayland_window_manager();

  // Triggering copy on touch-down event.
  for (auto send_input_event : send_input_event_closures) {
    ResetServerSelectionSource();

    send_input_event.Run();
    Sync();
    auto client_selection_serial = connection_->serial_tracker().GetSerial(
        {wl::SerialType::kTouchPress, wl::SerialType::kMousePress,
         wl::SerialType::kKeyPress});
    ASSERT_TRUE(client_selection_serial.has_value());

    // 1. Offer sample text as selection data.
    OfferData(GetBuffer(), kSampleClipboardText, {kMimeTypeTextUtf8});
    Sync();
    ASSERT_TRUE(GetServerSelectionSource());

    EXPECT_EQ(client_selection_serial->value,
              GetServerSelectionDevice()->selection_serial());

    // 2. Emulate an external client requesting to read the offered data and
    // make sure the appropriate string gets delivered.
    std::string delivered_text;
    base::MockCallback<wl::TestSelectionSource::ReadDataCallback> callback;
    EXPECT_CALL(callback, Run(_)).WillOnce([&](std::vector<uint8_t>&& data) {
      delivered_text = std::string(data.begin(), data.end());
    });
    GetServerSelectionSource()->ReadData(kMimeTypeTextUtf8, callback.Get());

    WaitForClipboardTasks();
    ASSERT_EQ(kSampleClipboardText, delivered_text);

    window_manager->SetPointerFocusedWindow(nullptr);
    window_manager->SetTouchFocusedWindow(nullptr);
    window_manager->SetKeyboardFocusedWindow(nullptr);
  }
}

TEST_P(WaylandClipboardTest, ReadFromClipboard) {
  // 1. Emulate a selection data offer coming in.
  auto* device = GetServerSelectionDevice();
  auto* data_offer = device->OnDataOffer();
  data_offer->OnOffer(kMimeTypeTextUtf8,
                      ToClipboardData(std::string(kSampleClipboardText)));
  device->OnSelection(data_offer);
  Sync();

  // 2. Request to read the offered data and check whether the read text matches
  // the previously offered one.
  std::string text;
  base::MockCallback<PlatformClipboard::RequestDataClosure> callback;
  EXPECT_CALL(callback, Run(_)).WillOnce([&](PlatformClipboard::Data data) {
    ASSERT_TRUE(data);
    text = std::string(data->front_as<const char>(), data->size());
  });

  clipboard_->RequestClipboardData(GetBuffer(), kMimeTypeTextUtf8,
                                   callback.Get());
  WaitForClipboardTasks();
  EXPECT_EQ(kSampleClipboardText, text);
}

// Regression test for crbug.com/1183939. Ensures unicode mime types take
// priority over text/plain when reading text.
TEST_P(WaylandClipboardTest, ReadFromClipboardPrioritizeUtf) {
  auto* data_offer = GetServerSelectionDevice()->OnDataOffer();
  data_offer->OnOffer(kMimeTypeText,
                      ToClipboardData(std::string("ascii_text")));
  data_offer->OnOffer(kMimeTypeTextUtf8,
                      ToClipboardData(std::string("utf8_text")));
  GetServerSelectionDevice()->OnSelection(data_offer);
  Sync();

  std::string text;
  base::MockCallback<PlatformClipboard::RequestDataClosure> callback;
  EXPECT_CALL(callback, Run(_)).WillOnce([&](PlatformClipboard::Data data) {
    ASSERT_TRUE(data);
    text = std::string(data->front_as<const char>(), data->size());
  });

  clipboard_->RequestClipboardData(GetBuffer(), kMimeTypeTextUtf8,
                                   callback.Get());
  WaitForClipboardTasks();
  EXPECT_EQ("utf8_text", text);
}

TEST_P(WaylandClipboardTest, ReadFromClipboardWithoutOffer) {
  // When no data offer is advertised and client requests clipboard data from
  // the server, the response callback should be gracefully called with null
  // data.
  base::MockCallback<PlatformClipboard::RequestDataClosure> callback;
  EXPECT_CALL(callback, Run(Eq(nullptr))).Times(1);
  clipboard_->RequestClipboardData(GetBuffer(), kMimeTypeTextUtf8,
                                   callback.Get());
}

TEST_P(WaylandClipboardTest, IsSelectionOwner) {
  connection_->serial_tracker().UpdateSerial(wl::SerialType::kMousePress, 1);

  OfferData(GetBuffer(), kSampleClipboardText, {kMimeTypeTextUtf8});
  Sync();
  ASSERT_TRUE(GetServerSelectionSource());
  ASSERT_TRUE(clipboard_->IsSelectionOwner(GetBuffer()));

  // The compositor sends OnCancelled whenever another application on the system
  // sets a new selection. It means we are not the application that owns the
  // current selection data.
  GetServerSelectionSource()->OnCancelled();
  Sync();

  ASSERT_FALSE(clipboard_->IsSelectionOwner(GetBuffer()));
  connection_->serial_tracker().ResetSerial(wl::SerialType::kMousePress);
}

// Ensures WaylandClipboard correctly handles overlapping read requests for
// different clipboard buffers.
TEST_P(WaylandClipboardTest, OverlapReadingFromDifferentBuffers) {
  // Offer a sample text as selection data.
  auto* data_offer = GetServerSelectionDevice()->OnDataOffer();
  data_offer->OnOffer(kMimeTypeTextUtf8,
                      ToClipboardData(std::string(kSampleClipboardText)));
  GetServerSelectionDevice()->OnSelection(data_offer);
  Sync();

  // Post a read request for the other buffer, which will start its execution
  // after the request above.
  auto other_buffer = GetBuffer() == ClipboardBuffer::kSelection
                          ? ClipboardBuffer::kCopyPaste
                          : ClipboardBuffer::kSelection;
  base::MockCallback<PlatformClipboard::RequestDataClosure> callback;
  EXPECT_CALL(callback, Run(Eq(nullptr))).Times(1);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&PlatformClipboard::RequestClipboardData,
                                base::Unretained(clipboard_), other_buffer,
                                kMimeTypeTextUtf8, callback.Get()));

  // Instantly start a clipboard read request for kCopyPaste buffer (the actual
  // data transfer will take place asynchronously. See WaylandDataDevice impl)
  // and ensure read callback is called with the expected resulting data,
  // regardless any other request that may arrive in the meantime.
  std::string text;
  base::MockCallback<PlatformClipboard::RequestDataClosure> got_text;
  EXPECT_CALL(got_text, Run(_)).WillOnce([&](PlatformClipboard::Data data) {
    ASSERT_NE(nullptr, data);
    text = std::string(data->front_as<const char>(), data->size());
  });
  clipboard_->RequestClipboardData(GetBuffer(), kMimeTypeTextUtf8,
                                   got_text.Get());

  WaitForClipboardTasks();
  EXPECT_EQ(kSampleClipboardText, text);
}

// Ensures clipboard change callback is fired only once per read/write.
TEST_P(WaylandClipboardTest, ClipboardChangeNotifications) {
  base::MockCallback<PlatformClipboard::ClipboardDataChangedCallback>
      clipboard_changed_callback;
  clipboard_->SetClipboardDataChangedCallback(clipboard_changed_callback.Get());
  const auto buffer = GetBuffer();

  // 1. For selection offered by an external application.
  EXPECT_CALL(clipboard_changed_callback, Run(buffer)).Times(1);
  auto* data_offer = GetServerSelectionDevice()->OnDataOffer();
  data_offer->OnOffer(kMimeTypeTextUtf8,
                      ToClipboardData(std::string(kSampleClipboardText)));
  GetServerSelectionDevice()->OnSelection(data_offer);
  Sync();
  EXPECT_FALSE(clipboard_->IsSelectionOwner(buffer));

  // 2. For selection offered by Chromium.
  connection_->serial_tracker().UpdateSerial(wl::SerialType::kMousePress, 1);
  EXPECT_CALL(clipboard_changed_callback, Run(buffer)).Times(1);
  OfferData(buffer, kSampleClipboardText, {kMimeTypeTextUtf8});
  Sync();
  ASSERT_TRUE(GetServerSelectionSource());
  EXPECT_TRUE(clipboard_->IsSelectionOwner(buffer));
  connection_->serial_tracker().ResetSerial(wl::SerialType::kMousePress);
}

// Verifies clipboard calls targeting primary selection buffer no-op and run
// gracefully when no primary selection protocol is available.
TEST_P(CopyPasteOnlyClipboardTest, PrimarySelectionRequestsNoop) {
  const auto buffer = ClipboardBuffer::kSelection;

  base::MockCallback<PlatformClipboard::OfferDataClosure> offer_done;
  EXPECT_CALL(offer_done, Run()).Times(1);
  clipboard_->OfferClipboardData(buffer, {}, offer_done.Get());
  EXPECT_FALSE(clipboard_->IsSelectionOwner(buffer));

  base::MockCallback<PlatformClipboard::RequestDataClosure> got_data;
  EXPECT_CALL(got_data, Run(IsNull())).Times(1);
  clipboard_->RequestClipboardData(buffer, kMimeTypeTextUtf8, got_data.Get());

  base::MockCallback<PlatformClipboard::GetMimeTypesClosure> got_mime_types;
  EXPECT_CALL(got_mime_types, Run(IsEmpty())).Times(1);
  clipboard_->GetAvailableMimeTypes(buffer, got_mime_types.Get());
}

// Makes sure overlapping read requests for the same clipboard buffer are
// properly handled.
// TODO(crbug.com/443355): Re-enable once Clipboard API becomes async.
TEST_P(CopyPasteOnlyClipboardTest, DISABLED_OverlappingReadRequests) {
  // Create an selection data offer containing plain and html mime types.
  auto* data_device = server_.data_device_manager()->data_device();
  auto* data_offer = data_device->OnDataOffer();
  data_offer->OnOffer(kMimeTypeText, ToClipboardData(std::string("text")));
  data_offer->OnOffer(kMimeTypeHTML, ToClipboardData(std::string("html")));
  data_device->OnSelection(data_offer);
  Sync();

  // Schedule a clipboard read task (for text/html mime type). As read requests
  // are processed asynchronously, this will actually start when the request
  // below is already under processing, thus emulating 2 "overlapping" requests.
  std::string html;
  base::MockCallback<PlatformClipboard::RequestDataClosure> got_html;
  EXPECT_CALL(got_html, Run(_)).WillOnce([&](PlatformClipboard::Data data) {
    html = std::string(data->front_as<const char>(), data->size());
  });
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlatformClipboard::RequestClipboardData,
                     base::Unretained(clipboard_), ClipboardBuffer::kCopyPaste,
                     kMimeTypeHTML, got_html.Get()));

  // Instantly start a read request for text/plain mime type.
  std::string text;
  base::MockCallback<PlatformClipboard::RequestDataClosure> got_text;
  EXPECT_CALL(got_text, Run(_)).WillOnce([&](PlatformClipboard::Data data) {
    text = std::string(data->front_as<const char>(), data->size());
  });
  clipboard_->RequestClipboardData(ClipboardBuffer::kCopyPaste, kMimeTypeText,
                                   got_text.Get());

  // Wait for clipboard tasks to complete and ensure both requests were
  // processed correctly.
  WaitForClipboardTasks();
  EXPECT_EQ("html", html);
  EXPECT_EQ("text", text);
}

INSTANTIATE_TEST_SUITE_P(
    WithZwpPrimarySelection,
    WaylandClipboardTest,
    Values(wl::ServerConfig{
        .primary_selection_protocol = wl::PrimarySelectionProtocol::kZwp}));

INSTANTIATE_TEST_SUITE_P(
    WithGtkPrimarySelection,
    WaylandClipboardTest,
    Values(wl::ServerConfig{
        .primary_selection_protocol = wl::PrimarySelectionProtocol::kGtk}));

INSTANTIATE_TEST_SUITE_P(
    WithoutPrimarySelection,
    WaylandClipboardTest,
    Values(wl::ServerConfig{
        .primary_selection_protocol = wl::PrimarySelectionProtocol::kNone}));

INSTANTIATE_TEST_SUITE_P(
    WithoutPrimarySelection,
    CopyPasteOnlyClipboardTest,
    Values(wl::ServerConfig{
        .primary_selection_protocol = wl::PrimarySelectionProtocol::kNone}));

}  // namespace ui
