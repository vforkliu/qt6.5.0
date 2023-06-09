// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_ZXDG_TOPLEVEL_V6_WRAPPER_IMPL_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_ZXDG_TOPLEVEL_V6_WRAPPER_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/ozone/platform/wayland/host/shell_toplevel_wrapper.h"

namespace ui {

class ZXDGSurfaceV6WrapperImpl;
class WaylandConnection;
class WaylandWindow;

// Toplevel wrapper for xdg-shell-unstable-v6
class ZXDGToplevelV6WrapperImpl : public ShellToplevelWrapper {
 public:
  ZXDGToplevelV6WrapperImpl(std::unique_ptr<ZXDGSurfaceV6WrapperImpl> surface,
                            WaylandWindow* wayland_window,
                            WaylandConnection* connection);
  ZXDGToplevelV6WrapperImpl(const ZXDGToplevelV6WrapperImpl&) = delete;
  ZXDGToplevelV6WrapperImpl& operator=(const ZXDGToplevelV6WrapperImpl&) =
      delete;
  ~ZXDGToplevelV6WrapperImpl() override;

  // ShellSurfaceWrapper overrides:
  bool Initialize() override;
  void SetMaximized() override;
  void UnSetMaximized() override;
  void SetFullscreen() override;
  void UnSetFullscreen() override;
  void SetMinimized() override;
  void SurfaceMove(WaylandConnection* connection) override;
  void SurfaceResize(WaylandConnection* connection, uint32_t hittest) override;
  void SetTitle(const std::u16string& title) override;
  void AckConfigure(uint32_t serial) override;
  bool IsConfigured() override;
  void SetWindowGeometry(const gfx::Rect& bounds) override;
  void SetMinSize(int32_t width, int32_t height) override;
  void SetMaxSize(int32_t width, int32_t height) override;
  void SetAppId(const std::string& app_id) override;
  void SetDecoration(DecorationMode decoration) override;
  void Lock(WaylandOrientationLockType lock_type) override;
  void Unlock() override;
  void RequestWindowBounds(const gfx::Rect& geometry) override;
  void SetRestoreInfo(int32_t restore_session_id,
                      int32_t restore_window_id) override;
  void SetRestoreInfoWithWindowIdSource(
      int32_t restore_session_id,
      const std::string& restore_window_id_source) override;
  void SetSystemModal(bool modal) override;
  bool SupportsScreenCoordinates() const override;
  void EnableScreenCoordinates() override;
  void SetFloat() override;
  void UnSetFloat() override;
  void SetZOrder(ZOrderLevel z_order) override;
  bool SupportsActivation() override;
  void Activate() override;
  void Deactivate() override;

  ZXDGSurfaceV6WrapperImpl* zxdg_surface_v6_wrapper() const;

 private:
  // zxdg_toplevel_v6_listener
  static void ConfigureTopLevel(void* data,
                                struct zxdg_toplevel_v6* zxdg_toplevel_v6,
                                int32_t width,
                                int32_t height,
                                struct wl_array* states);
  static void CloseTopLevel(void* data,
                            struct zxdg_toplevel_v6* zxdg_toplevel_v6);

  // Ground surface for this toplevel wrapper.
  std::unique_ptr<ZXDGSurfaceV6WrapperImpl> zxdg_surface_v6_wrapper_;

  // Non-owing WaylandWindow that uses this toplevel wrapper.
  const raw_ptr<WaylandWindow> wayland_window_;
  const raw_ptr<WaylandConnection> connection_;

  // XDG Shell V6 object.
  wl::Object<zxdg_toplevel_v6> zxdg_toplevel_v6_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_ZXDG_TOPLEVEL_V6_WRAPPER_IMPL_H_
