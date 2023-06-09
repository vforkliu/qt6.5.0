// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMAGE_NATIVE_PIXMAP_H_
#define UI_GL_GL_IMAGE_NATIVE_PIXMAP_H_

#include <stdint.h>

#include <string>

#include "ui/gfx/native_pixmap.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_image_egl.h"

namespace gl {

class GL_EXPORT GLImageNativePixmap : public gl::GLImageEGL {
 public:
  GLImageNativePixmap(const gfx::Size& size,
                      gfx::BufferFormat format,
                      gfx::BufferPlane plane = gfx::BufferPlane::DEFAULT);

  // Create an EGLImage from a given NativePixmap.
  bool Initialize(scoped_refptr<gfx::NativePixmap> pixmap);
  bool InitializeForOverlay(scoped_refptr<gfx::NativePixmap> pixmap);
  // Create an EGLImage from a given GL texture.
  bool InitializeFromTexture(uint32_t texture_id);
  // Export the wrapped EGLImage to dmabuf fds.
  gfx::NativePixmapHandle ExportHandle();

  // Overridden from GLImage:
  unsigned GetInternalFormat() override;
  unsigned GetDataType() override;
  bool BindTexImage(unsigned target) override;
  bool CopyTexImage(unsigned target) override;
  bool CopyTexSubImage(unsigned target,
                       const gfx::Point& offset,
                       const gfx::Rect& rect) override;
  void Flush() override;
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t process_tracing_id,
                    const std::string& dump_name) override;
  scoped_refptr<gfx::NativePixmap> GetNativePixmap() override;

 protected:
  ~GLImageNativePixmap() override;

 private:
  gfx::BufferFormat format_;
  scoped_refptr<gfx::NativePixmap> pixmap_;
  gfx::BufferPlane plane_;
  bool has_image_flush_external_;
  bool has_image_dma_buf_export_;
  bool did_initialize_;
};

}  // namespace gl

#endif  // UI_GL_GL_IMAGE_NATIVE_PIXMAP_H_
