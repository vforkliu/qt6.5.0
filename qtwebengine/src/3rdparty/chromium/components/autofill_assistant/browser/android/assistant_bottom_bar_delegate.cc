// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill_assistant/browser/android/assistant_bottom_bar_delegate.h"

#include "components/autofill_assistant/android/jni_headers/AssistantBottomBarNativeDelegate_jni.h"
#include "components/autofill_assistant/browser/android/ui_controller_android.h"

using base::android::AttachCurrentThread;

namespace autofill_assistant {

AssistantBottomBarDelegate::AssistantBottomBarDelegate(
    UiControllerAndroid* ui_controller)
    : ui_controller_(ui_controller) {
  java_assistant_bottom_bar_delegate_ =
      Java_AssistantBottomBarNativeDelegate_create(
          AttachCurrentThread(), reinterpret_cast<intptr_t>(this));
}

AssistantBottomBarDelegate::~AssistantBottomBarDelegate() {
  Java_AssistantBottomBarNativeDelegate_clearNativePtr(
      AttachCurrentThread(), java_assistant_bottom_bar_delegate_);
}

bool AssistantBottomBarDelegate::OnBackButtonClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  return ui_controller_->OnBackButtonClicked();
}

void AssistantBottomBarDelegate::OnBottomSheetClosedWithSwipe(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  ui_controller_->OnBottomSheetClosedWithSwipe();
}

base::android::ScopedJavaGlobalRef<jobject>
AssistantBottomBarDelegate::GetJavaObject() {
  return java_assistant_bottom_bar_delegate_;
}

}  // namespace autofill_assistant
