// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CONSTANTS_CONSTANTS_UTIL_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CONSTANTS_CONSTANTS_UTIL_H_

#include <vector>

#include "chrome/browser/ui/webui/settings/chromeos/constants/routes.mojom.h"
#include "chrome/browser/ui/webui/settings/chromeos/constants/setting.mojom.h"

namespace chromeos {
namespace settings {
namespace constants {

const std::vector<mojom::Section>& AllSections();
const std::vector<mojom::Subpage>& AllSubpages();
const std::vector<mojom::Setting>& AllSettings();

}  // namespace constants
}  // namespace settings
}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when it moved to ash.
namespace ash::settings::constants {
using ::chromeos::settings::constants::AllSections;
using ::chromeos::settings::constants::AllSettings;
using ::chromeos::settings::constants::AllSubpages;
}  // namespace ash::settings::constants

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CONSTANTS_CONSTANTS_UTIL_H_
