// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_FORMAT_UTILS_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_FORMAT_UTILS_H_

#include "build/build_config.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/shared_image_format.h"
#include "components/viz/common/viz_resource_format_export.h"
#include "gpu/vulkan/buildflags.h"
#include "skia/buildflags.h"
#include "third_party/dawn/include/dawn/webgpu.h"
#include "third_party/dawn/include/dawn/webgpu_cpp.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/buffer_types.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include <vulkan/vulkan_core.h>
#endif

namespace viz {

VIZ_RESOURCE_FORMAT_EXPORT SkColorType
ResourceFormatToClosestSkColorType(bool gpu_compositing, ResourceFormat format);

VIZ_RESOURCE_FORMAT_EXPORT int BitsPerPixel(ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT bool HasAlpha(ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT ResourceFormat
SkColorTypeToResourceFormat(SkColorType color_type);

// The following functions use unsigned int instead of GLenum, since including
// third_party/khronos/GLES2/gl2.h causes redefinition errors as
// macros/functions defined in it conflict with macros/functions defined in
// ui/gl/gl_bindings.h. See http://crbug.com/512833 for more information.
VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLDataType(ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLDataFormat(ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLInternalFormat(ResourceFormat format);

// Returns the pixel format of the resource when mapped into client-side memory.
// Returns a default value when IsGpuMemoryBufferFormatSupported() returns false
// for a given format, as in this case the resource will not be mapped into
// client-side memory, and the returned value is not used.
VIZ_RESOURCE_FORMAT_EXPORT gfx::BufferFormat BufferFormat(
    ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT bool IsResourceFormatCompressed(
    ResourceFormat format);

// |use_angle_rgbx_format| should be true when the GL_ANGLE_rgbx_internal_format
// extension is available.
VIZ_RESOURCE_FORMAT_EXPORT unsigned int TextureStorageFormat(
    ResourceFormat format,
    bool use_angle_rgbx_format);

// Returns whether the format can be used with GpuMemoryBuffer texture storage.
VIZ_RESOURCE_FORMAT_EXPORT bool IsGpuMemoryBufferFormatSupported(
    ResourceFormat format);

// Returns whether the format can be used as a software bitmap for export to the
// display compositor.
VIZ_RESOURCE_FORMAT_EXPORT bool IsBitmapFormatSupported(ResourceFormat format);

VIZ_RESOURCE_FORMAT_EXPORT ResourceFormat
GetResourceFormat(gfx::BufferFormat format);

VIZ_RESOURCE_FORMAT_EXPORT bool GLSupportsFormat(ResourceFormat format);

#if BUILDFLAG(ENABLE_VULKAN)
VIZ_RESOURCE_FORMAT_EXPORT bool HasVkFormat(ResourceFormat format);
VIZ_RESOURCE_FORMAT_EXPORT VkFormat ToVkFormat(ResourceFormat format);
#endif

VIZ_RESOURCE_FORMAT_EXPORT wgpu::TextureFormat ToDawnFormat(
    ResourceFormat format);
// Same as ToDawnFormat, except it casts from wgpu::TextureFormat to
// WGPUTextureFormat instead.
VIZ_RESOURCE_FORMAT_EXPORT WGPUTextureFormat
ToWGPUFormat(ResourceFormat format);

#if BUILDFLAG(IS_APPLE)
VIZ_RESOURCE_FORMAT_EXPORT unsigned int ToMTLPixelFormat(ResourceFormat format);
#endif

// Overload all above functions with SharedImageFormat in place of
// ResourceFormat.
// WARNING: The `format` must be single planar.
// TODO(hitawala): Add multiplanar format support. Also, move util functions
// that are not needed by clients to SharedImageFormat.

VIZ_RESOURCE_FORMAT_EXPORT SkColorType
ResourceFormatToClosestSkColorType(bool gpu_compositing,
                                   SharedImageFormat format);

VIZ_RESOURCE_FORMAT_EXPORT int BitsPerPixel(SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT bool HasAlpha(SharedImageFormat format);

VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLDataType(SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLDataFormat(SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT unsigned int GLInternalFormat(
    SharedImageFormat format);

VIZ_RESOURCE_FORMAT_EXPORT gfx::BufferFormat BufferFormat(
    SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT bool IsResourceFormatCompressed(
    SharedImageFormat format);

VIZ_RESOURCE_FORMAT_EXPORT unsigned int TextureStorageFormat(
    SharedImageFormat format,
    bool use_angle_rgbx_format);

VIZ_RESOURCE_FORMAT_EXPORT bool IsGpuMemoryBufferFormatSupported(
    SharedImageFormat format);

VIZ_RESOURCE_FORMAT_EXPORT bool GLSupportsFormat(SharedImageFormat format);

#if BUILDFLAG(ENABLE_VULKAN)
VIZ_RESOURCE_FORMAT_EXPORT bool HasVkFormat(SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT VkFormat ToVkFormat(SharedImageFormat format);
#endif

VIZ_RESOURCE_FORMAT_EXPORT wgpu::TextureFormat ToDawnFormat(
    SharedImageFormat format);
VIZ_RESOURCE_FORMAT_EXPORT WGPUTextureFormat
ToWGPUFormat(SharedImageFormat format);

#if BUILDFLAG(IS_APPLE)
VIZ_RESOURCE_FORMAT_EXPORT unsigned int ToMTLPixelFormat(
    SharedImageFormat format);
#endif

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_FORMAT_UTILS_H_
