// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_SCREEN_AI_PUBLIC_CPP_UTILITIES_H_
#define COMPONENTS_SERVICES_SCREEN_AI_PUBLIC_CPP_UTILITIES_H_

#include "base/files/file_path.h"

namespace screen_ai {

// Get the absolute path of the ScreenAI component.
base::FilePath GetLatestComponentBinaryPath();

// Returns the install directory relative to components folder.
base::FilePath GetRelativeInstallDir();

// Returns the folder in which ScreenAI component is installed.
base::FilePath GetComponentDir();

// Stores the path to the component binary. This value is kept in memory and is
// not kept between sessions or shared between processes.
void StoreComponentBinaryPath(const base::FilePath& path);

// Returns the component binary path if it is already stored by
// |StoreComponentBinaryPath|.
base::FilePath GetStoredComponentBinaryPath();

}  // namespace screen_ai
#endif  // COMPONENTS_SERVICES_SCREEN_AI_PUBLIC_CPP_UTILITIES_H_
