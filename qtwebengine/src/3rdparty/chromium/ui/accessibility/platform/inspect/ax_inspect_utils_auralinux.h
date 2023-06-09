// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_INSPECT_AX_INSPECT_UTILS_AURALINUX_H_
#define UI_ACCESSIBILITY_PLATFORM_INSPECT_AX_INSPECT_UTILS_AURALINUX_H_

#include <atk/atk.h>
#include <atspi/atspi.h>

#include "base/values.h"
#include "ui/accessibility/ax_export.h"

namespace ui {
struct AXTreeSelector;

AX_EXPORT const char* ATSPIStateToString(AtspiStateType state);
AX_EXPORT const char* ATSPIRelationToString(AtspiRelationType relation);
AX_EXPORT const char* ATSPIRoleToString(AtspiRole role);
AX_EXPORT const char* AtkRoleToString(AtkRole role);
AX_EXPORT std::vector<AtspiAccessible*> ChildrenOf(AtspiAccessible* node);
AX_EXPORT AtspiAccessible* FindAccessible(const AXTreeSelector&);
AX_EXPORT std::string GetDOMId(AtspiAccessible* node);

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_INSPECT_AX_INSPECT_UTILS_AURALINUX_H_
