// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_overlay.h"

#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_image.h"

namespace gl {

GLSurfaceOverlay::GLSurfaceOverlay(
    GLImage* image,
    std::unique_ptr<gfx::GpuFence> gpu_fence,
    const gfx::OverlayPlaneData& overlay_plane_data)
    : image_(image),
      gpu_fence_(std::move(gpu_fence)),
      overlay_plane_data_(overlay_plane_data) {}

GLSurfaceOverlay::GLSurfaceOverlay(GLSurfaceOverlay&& other) = default;

GLSurfaceOverlay::~GLSurfaceOverlay() {}

bool GLSurfaceOverlay::ScheduleOverlayPlane(gfx::AcceleratedWidget widget) {
  std::vector<gfx::GpuFence> acquire_fences;
  if (gpu_fence_)
    acquire_fences.push_back(std::move(*gpu_fence_));

  auto pixmap = image_->GetNativePixmap();
  DCHECK(pixmap);
  return pixmap->ScheduleOverlayPlane(widget, overlay_plane_data_,
                                      std::move(acquire_fences), {});
}

void GLSurfaceOverlay::Flush() const {
  return image_->Flush();
}

}  // namespace gl
