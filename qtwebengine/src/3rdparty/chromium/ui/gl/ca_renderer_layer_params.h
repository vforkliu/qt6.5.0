// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_CA_RENDERER_LAYER_PARAMS_H_
#define UI_GL_CA_RENDERER_LAYER_PARAMS_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/rrect_f.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/hdr_metadata.h"
#include "ui/gfx/mac/io_surface.h"
#include "ui/gfx/video_types.h"
#include "ui/gl/gl_export.h"

namespace gl {
class GLImage;
}

namespace ui {

// Mirrors core animation layer edge constants.
enum CALayerEdge : uint32_t {
  kLayerEdgeLeft = 0x1,
  kLayerEdgeRight = 0x2,
  kLayerEdgeBottom = 0x4,
  kLayerEdgeTop = 0x8,
};

struct GL_EXPORT CARendererLayerParams {
  CARendererLayerParams(bool is_clipped,
                        const gfx::Rect clip_rect,
                        const gfx::RRectF rounded_corner_bounds,
                        unsigned sorting_context_id,
                        const gfx::Transform& transform,
                        gfx::ScopedIOSurface io_surface,
                        const gfx::ColorSpace& io_surface_color_space,
                        const gfx::RectF& contents_rect,
                        const gfx::Rect& rect,
                        unsigned background_color,
                        unsigned edge_aa_mask,
                        float opacity,
                        unsigned filter,
                        absl::optional<gfx::HDRMetadata> hdr_metadata,
                        gfx::ProtectedVideoType protected_video_type);
  // TODO(crbug.com/1366805): remove this ctor when it is not used anywhere.
  CARendererLayerParams(bool is_clipped,
                        const gfx::Rect clip_rect,
                        const gfx::RRectF rounded_corner_bounds,
                        unsigned sorting_context_id,
                        const gfx::Transform& transform,
                        gl::GLImage* image,
                        const gfx::RectF& contents_rect,
                        const gfx::Rect& rect,
                        unsigned background_color,
                        unsigned edge_aa_mask,
                        float opacity,
                        unsigned filter,
                        absl::optional<gfx::HDRMetadata> hdr_metadata,
                        gfx::ProtectedVideoType protected_video_type);
  CARendererLayerParams(const CARendererLayerParams& other);
  ~CARendererLayerParams();

  bool is_clipped;
  const gfx::Rect clip_rect;
  const gfx::RRectF rounded_corner_bounds;
  unsigned sorting_context_id;
  const gfx::Transform transform;
  gfx::ScopedIOSurface io_surface;
  gfx::ColorSpace io_surface_color_space;
  const gfx::RectF contents_rect;
  const gfx::Rect rect;
  unsigned background_color;
  unsigned edge_aa_mask;
  float opacity;
  unsigned filter;
  absl::optional<gfx::HDRMetadata> hdr_metadata;
  gfx::ProtectedVideoType protected_video_type;

  // This is a subset of cc::FilterOperation::FilterType.
  enum class FilterEffectType : uint32_t {
    GRAYSCALE,
    SEPIA,
    SATURATE,
    HUE_ROTATE,
    INVERT,
    BRIGHTNESS,
    CONTRAST,
    OPACITY,
    BLUR,
    DROP_SHADOW,
  };
  struct GL_EXPORT FilterEffect {
    FilterEffectType type = FilterEffectType::GRAYSCALE;

    // For every filter other than DROP_SHADOW, only |amount| is populated.
    float amount = 0;
    gfx::Point drop_shadow_offset;
    SkColor drop_shadow_color = 0;
  };
  using FilterEffects = std::vector<FilterEffect>;

  FilterEffects filter_effects;
};

}  // namespace ui

#endif  // UI_GL_CA_RENDERER_LAYER_PARAMS_H_
