// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/containers/flat_map.h"
#include "components/autofill_assistant/android/jni_headers/BaseOnboardingCoordinator_jni.h"
#include "components/autofill_assistant/browser/android/onboarding_fetcher_factory.h"
#include "components/autofill_assistant/browser/autofill_assistant_onboarding_fetcher.h"

namespace autofill_assistant {
namespace {

void UpdateView(
    JNIEnv* env,
    const base::android::ScopedJavaGlobalRef<jobject> jonboarding_coordinator,
    const base::flat_map<std::string, std::string>& string_map) {
  for (const auto& it : string_map) {
    DCHECK(!it.first.empty());
    DCHECK(!it.second.empty());
    Java_BaseOnboardingCoordinator_addEntryToStringMap(
        env, jonboarding_coordinator,
        base::android::ConvertUTF8ToJavaString(env, it.first),
        base::android::ConvertUTF8ToJavaString(env, it.second));
  }
  Java_BaseOnboardingCoordinator_updateAndShowView(env,
                                                   jonboarding_coordinator);
}

}  // namespace

// static
void JNI_BaseOnboardingCoordinator_FetchOnboardingDefinition(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jonboarding_coordinator,
    const base::android::JavaParamRef<jstring>& jintent,
    const base::android::JavaParamRef<jstring>& jlocale,
    jlong browser_context_ptr,
    jint timeout_ms) {
  if (!jonboarding_coordinator || !jintent || !jlocale || !timeout_ms) {
    Java_BaseOnboardingCoordinator_updateAndShowView(env,
                                                     jonboarding_coordinator);
    return;
  }

  OnboardingFetcherFactory::GetInstance()
      ->GetForBrowserContext(static_cast<content::BrowserContext*>(
          reinterpret_cast<void*>(browser_context_ptr)))
      ->FetchOnboardingDefinition(
          base::android::ConvertJavaStringToUTF8(env, jintent),
          base::android::ConvertJavaStringToUTF8(env, jlocale),
          static_cast<int>(timeout_ms),
          base::BindOnce(&UpdateView, env,
                         base::android::ScopedJavaGlobalRef<jobject>(
                             jonboarding_coordinator)));
}

}  // namespace autofill_assistant
