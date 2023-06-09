// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/debug/debug_overlay_handler.h"

#include "ash/constants/ash_switches.h"
#include "ash/public/cpp/style/dark_light_mode_controller.h"
#include "ash/shell.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/system/sys_info.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "chrome/common/chrome_paths.h"
#include "ui/display/display_switches.h"
#include "ui/snapshot/snapshot.h"

// Enable VLOG level 1.
#undef ENABLED_VLOG_LEVEL
#define ENABLED_VLOG_LEVEL 1

namespace chromeos {

namespace {

// A helper function which saves screenshot to the file.
void StoreScreenshot(const base::FilePath& screenshot_dir,
                     const std::string& screenshot_name,
                     scoped_refptr<base::RefCountedMemory> png_data) {
  if (!base::CreateDirectory(screenshot_dir)) {
    LOG(ERROR) << "Failed to create screenshot dir at "
               << screenshot_dir.value();
    return;
  }
  base::FilePath file_path = screenshot_dir.Append(screenshot_name);

  if (static_cast<size_t>(base::WriteFile(
          file_path, reinterpret_cast<const char*>(png_data->front()),
          static_cast<int>(png_data->size()))) != png_data->size()) {
    LOG(ERROR) << "Failed to save screenshot to " << file_path.value();
  } else {
    VLOG(1) << "Saved screenshot to " << file_path.value();
  }
}

// A helper function which invokes StoreScreenshot on TaskRunner.
void RunStoreScreenshotOnTaskRunner(
    const base::FilePath& screenshot_dir,
    const std::string& screenshot_name,
    scoped_refptr<base::RefCountedMemory> png_data) {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&StoreScreenshot, screenshot_dir, screenshot_name,
                     png_data));
}

}  // namespace
// DebugOverlayHandler, public: -----------------------------------------------

DebugOverlayHandler::DebugOverlayHandler() {
  // Rules for base directory:
  // 1) If command-line switch is specified, use the directory
  // 2) else if chromeos-on-linux case create OOBE_Screenshots in user-data-dir
  // 3) else (running on real device) create OOBE_Screenshots in /tmp
  base::FilePath base_dir;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kOobeScreenshotDirectory)) {
    base_dir =
        command_line->GetSwitchValuePath(switches::kOobeScreenshotDirectory);
  } else {
    if (base::SysInfo::IsRunningOnChromeOS()) {
      if (!base::GetTempDir(&base_dir)) {
        LOG(ERROR) << "Could not get Temp dir";
      }
    } else {
      // use user-data-dir as base directory when running chromeos-on-linux.
      if (!base::PathService::Get(chrome::DIR_USER_DATA, &base_dir)) {
        LOG(ERROR) << "User Data Directory not found";
      }
    }
    base_dir = base_dir.Append("OOBE_Screenshots");
  }

  add_resolution_to_filename_ =
      command_line->HasSwitch(::switches::kHostWindowBounds);

  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);
  std::string series_name =
      base::StringPrintf("%d-%02d-%02d - %02d.%02d.%02d", now.year, now.month,
                         now.day_of_month, now.hour, now.minute, now.second);
  screenshot_dir_ = base_dir.Append(series_name);
}

DebugOverlayHandler::~DebugOverlayHandler() = default;

void DebugOverlayHandler::DeclareJSCallbacks() {
  AddCallback("debug.captureScreenshot",
              &DebugOverlayHandler::HandleCaptureScreenshot);
  AddCallback("debug.toggleColorMode", &DebugOverlayHandler::ToggleColorMode);
}

void DebugOverlayHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {}

void DebugOverlayHandler::InitializeDeprecated() {}

// DebugOverlayHandler, private: ----------------------------------------------

void DebugOverlayHandler::HandleCaptureScreenshot(const std::string& name) {
  aura::Window::Windows root_windows = ash::Shell::GetAllRootWindows();
  if (root_windows.size() == 0)
    return;

  screenshot_index_++;
  std::string filename_base =
      base::StringPrintf("%04d - %s", screenshot_index_, name.c_str());

  for (size_t screen = 0; screen < root_windows.size(); ++screen) {
    aura::Window* root_window = root_windows[screen];
    gfx::Rect rect = root_window->bounds();
    std::string filename = filename_base;
    if (root_windows.size() > 1) {
      filename.append(base::StringPrintf("- Display %zu", screen));
    }

    if (add_resolution_to_filename_)
      filename.append("_" + rect.size().ToString());

    if (ash::DarkLightModeController::Get()->IsDarkModeEnabled()) {
      filename.append("_dark");
    }

    filename.append(".png");
    ui::GrabWindowSnapshotAsyncPNG(
        root_window, rect,
        base::BindOnce(&RunStoreScreenshotOnTaskRunner, screenshot_dir_,
                       filename));
  }
}

void DebugOverlayHandler::ToggleColorMode() {
  ash::DarkLightModeController::Get()->SetDarkModeEnabledForTest(  // IN-TEST
      !ash::DarkLightModeController::Get()->IsDarkModeEnabled());
}

}  // namespace chromeos
