// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ANDROID_ASSISTANT_HEADER_DELEGATE_H_
#define COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ANDROID_ASSISTANT_HEADER_DELEGATE_H_

#include "base/android/scoped_java_ref.h"
#include "base/memory/raw_ptr.h"

namespace autofill_assistant {
class UiControllerAndroid;
// Delegate class for the assistant header, to react on clicks on its buttons.
class AssistantHeaderDelegate {
 public:
  explicit AssistantHeaderDelegate(UiControllerAndroid* ui_controller);
  ~AssistantHeaderDelegate();

  void OnFeedbackButtonClicked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  void OnTtsButtonClicked(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& jcaller);

  base::android::ScopedJavaGlobalRef<jobject> GetJavaObject() const;

 private:
  raw_ptr<UiControllerAndroid> ui_controller_;

  // Java-side AssistantHeaderDelegate object.
  base::android::ScopedJavaGlobalRef<jobject> java_assistant_header_delegate_;
};
}  // namespace autofill_assistant

#endif  // COMPONENTS_AUTOFILL_ASSISTANT_BROWSER_ANDROID_ASSISTANT_HEADER_DELEGATE_H_
