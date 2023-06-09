// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ACTIONS_UNSUPPORTED_ACTION_H_
#define COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ACTIONS_UNSUPPORTED_ACTION_H_

#include "components/autofill_assistant/browser/actions/action.h"

namespace autofill_assistant {
// An unsupported action that always fails.
class UnsupportedAction : public Action {
 public:
  explicit UnsupportedAction(ActionDelegate* delegate,
                             const ActionProto& proto);

  UnsupportedAction(const UnsupportedAction&) = delete;
  UnsupportedAction& operator=(const UnsupportedAction&) = delete;

  ~UnsupportedAction() override;

 private:
  // Overrides Action:
  void InternalProcessAction(ProcessActionCallback callback) override;
};

}  // namespace autofill_assistant
#endif  // COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ACTIONS_UNSUPPORTED_ACTION_H_
