// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_CHROMEOS_GL_IMAGE_PROCESSOR_BACKEND_H_
#define MEDIA_GPU_CHROMEOS_GL_IMAGE_PROCESSOR_BACKEND_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/common/gl2_types.h"
#include "media/gpu/chromeos/image_processor_backend.h"
#include "media/gpu/media_gpu_export.h"

namespace gl {
class GLContext;
class GLSurface;
}  // namespace gl

namespace media {

// An image processor which uses GL to perform scaling.
class MEDIA_GPU_EXPORT GLImageProcessorBackend : public ImageProcessorBackend {
 public:
  static std::unique_ptr<ImageProcessorBackend> Create(
      const PortConfig& input_config,
      const PortConfig& output_config,
      OutputMode output_mode,
      VideoRotation relative_rotation,
      ErrorCB error_cb,
      scoped_refptr<base::SequencedTaskRunner> backend_task_runner);

  // ImageProcessorBackend implementation.
  void Process(scoped_refptr<VideoFrame> input_frame,
               scoped_refptr<VideoFrame> output_frame,
               FrameReadyCB cb) override;

 private:
  // Callback for initialization.
  using InitCB = base::OnceCallback<void(bool)>;

  GLImageProcessorBackend(
      const PortConfig& input_config,
      const PortConfig& output_config,
      OutputMode output_mode,
      VideoRotation relative_rotation,
      ErrorCB error_cb,
      scoped_refptr<base::SequencedTaskRunner> backend_task_runner);
  GLImageProcessorBackend(const GLImageProcessorBackend&) = delete;
  ~GLImageProcessorBackend() override;
  GLImageProcessorBackend& operator=(const GLImageProcessorBackend&) = delete;

  static bool IsSupported(const PortConfig& input_config,
                          const PortConfig& output_config,
                          VideoRotation relative_rotation);

  bool InitializeTask();
  void DestroyTask();

  scoped_refptr<gl::GLSurface> gl_surface_;
  scoped_refptr<gl::GLContext> gl_context_;

  GLuint src_texture_id_ = 0u;
  GLuint dst_texture_id_ = 0u;
  GLuint fb_id_ = 0u;

  const static int kTileWidth = 16;
  const static int kTileHeight = 32;
};

}  // namespace media

#endif  // MEDIA_GPU_CHROMEOS_GL_IMAGE_PROCESSOR_BACKEND_H_
