// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/mojom/video_encoder_info_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<media::mojom::ResolutionBitrateLimitDataView,
                  media::ResolutionBitrateLimit>::
    Read(media::mojom::ResolutionBitrateLimitDataView data,
         media::ResolutionBitrateLimit* out) {
  if (!data.ReadFrameSize(&out->frame_size))
    return false;
  out->min_start_bitrate_bps = data.min_start_bitrate_bps();
  out->min_bitrate_bps = data.min_bitrate_bps();
  out->max_bitrate_bps = data.max_bitrate_bps();
  return true;
}

// static
bool StructTraits<
    media::mojom::VideoEncoderInfoDataView,
    media::VideoEncoderInfo>::Read(media::mojom::VideoEncoderInfoDataView data,
                                   media::VideoEncoderInfo* out) {
  out->supports_native_handle = data.supports_native_handle();
  out->has_trusted_rate_controller = data.has_trusted_rate_controller();
  out->is_hardware_accelerated = data.is_hardware_accelerated();
  out->supports_simulcast = data.supports_simulcast();
  out->reports_average_qp = data.reports_average_qp();

  if (!data.ReadImplementationName(&out->implementation_name))
    return false;

  base::span<std::vector<uint8_t>> fps_allocation(out->fps_allocation);
  if (!data.ReadFpsAllocation(&fps_allocation))
    return false;

  if (!data.ReadResolutionBitrateLimits(&out->resolution_bitrate_limits))
    return false;

  return true;
}
}  // namespace mojo
