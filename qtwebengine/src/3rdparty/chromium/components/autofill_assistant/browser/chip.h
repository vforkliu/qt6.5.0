// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_CHIP_H_
#define COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_CHIP_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "components/autofill_assistant/browser/service.pb.h"

namespace autofill_assistant {
class UserAction;  // For SetDefaultChipType

// A structure to represent a Chip shown in the carousel.
//
// Might be empty.
struct Chip {
  explicit Chip(const ChipProto& proto);
  Chip();
  Chip(const Chip& other);
  ~Chip();

  bool empty() const;

  ChipType type = UNKNOWN_CHIP_TYPE;

  ChipIcon icon = NO_ICON;

  // Localized string to display.
  std::string text;

  // Whether this chip is sticky. A sticky chip will be a candidate to be
  // displayed in the header if the peek mode of the sheet is HANDLE_HEADER.
  bool sticky = false;

  // Whether this chip should be displayed in the carousel.
  bool visible = true;

  // Accessibility string for chip.
  std::string content_description;

  // True when accessibility string for chip is set.
  bool is_content_description_set;
};

// Guarantees that the Chip.type of all chips is set to a sensible value.
void SetDefaultChipType(std::vector<UserAction>* chips);

}  // namespace autofill_assistant

#endif  // COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_CHIP_H_
