// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_DEVICE_STORAGE_UTIL_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_DEVICE_STORAGE_UTIL_H_

#include <cstdint>

namespace chromeos::settings {

// Round |bytes| to the next power of 2, where the next power of 2 is greater
// than or equal to |bytes|.
// RoundByteSize(3) will return 4.
// RoundByteSize(4) will return 4.
int64_t RoundByteSize(int64_t bytes);

}  // namespace chromeos::settings

// TODO(https://crbug.com/1164001): remove when it moved to ash.
namespace ash::settings {
using ::chromeos::settings::RoundByteSize;
}

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_DEVICE_STORAGE_UTIL_H_
