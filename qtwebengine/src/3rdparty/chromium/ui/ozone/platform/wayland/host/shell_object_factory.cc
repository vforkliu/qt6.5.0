// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/shell_object_factory.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/xdg_popup_wrapper_impl.h"
#include "ui/ozone/platform/wayland/host/xdg_surface_wrapper_impl.h"
#include "ui/ozone/platform/wayland/host/xdg_toplevel_wrapper_impl.h"
#include "ui/ozone/platform/wayland/host/zxdg_popup_v6_wrapper_impl.h"
#include "ui/ozone/platform/wayland/host/zxdg_surface_v6_wrapper_impl.h"
#include "ui/ozone/platform/wayland/host/zxdg_toplevel_v6_wrapper_impl.h"

namespace ui {

namespace {

void ReportBaseShellUMA(UMALinuxWaylandShell shell) {
  static bool used_shell_reported = false;
  if (used_shell_reported)
    return;
  base::UmaHistogramEnumeration("Linux.Wayland.BaseShellUsed", shell);
  used_shell_reported = true;
}

}  // namespace

ShellObjectFactory::ShellObjectFactory() = default;
ShellObjectFactory::~ShellObjectFactory() = default;

std::unique_ptr<ShellToplevelWrapper>
ShellObjectFactory::CreateShellToplevelWrapper(WaylandConnection* connection,
                                               WaylandWindow* wayland_window) {
  if (connection->shell()) {
    ReportBaseShellUMA(UMALinuxWaylandShell::kXdgWmBase);

    auto surface =
        std::make_unique<XDGSurfaceWrapperImpl>(wayland_window, connection);
    if (!surface->Initialize())
      return nullptr;

    auto toplevel = std::make_unique<XDGToplevelWrapperImpl>(
        std::move(surface), wayland_window, connection);
    return toplevel->Initialize() ? std::move(toplevel) : nullptr;
  } else if (connection->shell_v6()) {
    ReportBaseShellUMA(UMALinuxWaylandShell::kXdgShellV6);

    auto surface =
        std::make_unique<ZXDGSurfaceV6WrapperImpl>(wayland_window, connection);
    if (!surface->Initialize())
      return nullptr;

    auto toplevel = std::make_unique<ZXDGToplevelV6WrapperImpl>(
        std::move(surface), wayland_window, connection);
    return toplevel->Initialize() ? std::move(toplevel) : nullptr;
  }
  LOG(WARNING) << "Shell protocol is not available.";
  return nullptr;
}

std::unique_ptr<ShellPopupWrapper> ShellObjectFactory::CreateShellPopupWrapper(
    WaylandConnection* connection,
    WaylandWindow* wayland_window,
    const ShellPopupParams& params) {
  if (connection->shell()) {
    auto surface =
        std::make_unique<XDGSurfaceWrapperImpl>(wayland_window, connection);
    if (!surface->Initialize())
      return nullptr;

    auto popup = std::make_unique<XDGPopupWrapperImpl>(
        std::move(surface), wayland_window, connection);
    return popup->Initialize(params) ? std::move(popup) : nullptr;
  } else if (connection->shell_v6()) {
    auto surface =
        std::make_unique<ZXDGSurfaceV6WrapperImpl>(wayland_window, connection);
    if (!surface->Initialize())
      return nullptr;

    auto popup = std::make_unique<ZXDGPopupV6WrapperImpl>(
        std::move(surface), wayland_window, connection);
    return popup->Initialize(params) ? std::move(popup) : nullptr;
  }
  LOG(WARNING) << "Shell protocol is not available.";
  return nullptr;
}

}  // namespace ui
