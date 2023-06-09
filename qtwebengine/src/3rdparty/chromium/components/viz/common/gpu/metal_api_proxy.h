// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GPU_METAL_API_PROXY_H_
#define COMPONENTS_VIZ_COMMON_GPU_METAL_API_PROXY_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"

#import <Metal/Metal.h>

#include "base/mac/scoped_nsobject.h"

namespace gl {
class ProgressReporter;
}  // namespace gl

class MTLLibraryCache;

// The MTLDeviceProxy wraps all calls to an MTLDevice. It reports progress
// to the GPU watchdog to prevent the watchdog from killing the GPU process
// when progress is being made.
@interface MTLDeviceProxy : NSObject <MTLDevice> {
  base::scoped_nsprotocol<id<MTLDevice>> _device;

  // Weak pointer to the most vertexMain and fragmentMain MTLFunctions most
  // recently present in the result from a -newLibraryWithSource. Used for
  // comparison only in -newRenderPipelineStateWithDescriptor.
  // https://crbug.com/974219
  id _vertexSourceFunction;
  id _fragmentSourceFunction;

  // The source used in the -newLibraryWithSource functions that created
  // the above functions.
  std::string _vertexSource;
  std::string _fragmentSource;

  // Weak pointer to the progress reporter used to avoid watchdog timeouts.
  // This must be re-set to nullptr when it is no longer known to be valid.
  raw_ptr<gl::ProgressReporter> _progressReporter;

  std::unique_ptr<MTLLibraryCache> _libraryCache;
}

- (id)initWithDevice:(id<MTLDevice>)device;
- (void)setProgressReporter:(gl::ProgressReporter*)progressReporter;
@end

#endif  // COMPONENTS_VIZ_COMMON_GPU_METAL_API_PROXY_H_
