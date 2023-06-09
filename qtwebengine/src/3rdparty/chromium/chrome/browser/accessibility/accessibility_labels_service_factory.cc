// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/accessibility/accessibility_labels_service_factory.h"

#include "build/chromeos_buildflags.h"
#include "chrome/browser/accessibility/accessibility_labels_service.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/profiles/profile_helper.h"
#endif

// static
AccessibilityLabelsService* AccessibilityLabelsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AccessibilityLabelsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AccessibilityLabelsService*
AccessibilityLabelsServiceFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<AccessibilityLabelsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/false));
}

// static
AccessibilityLabelsServiceFactory*
AccessibilityLabelsServiceFactory::GetInstance() {
  return base::Singleton<AccessibilityLabelsServiceFactory>::get();
}

AccessibilityLabelsServiceFactory::AccessibilityLabelsServiceFactory()
    : ProfileKeyedServiceFactory(
          "AccessibilityLabelsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Use OTR profile for Guest Session.
              .WithGuest(ProfileSelection::kOffTheRecordOnly)
              // No service for system profile.
              .WithSystem(ProfileSelection::kNone)
              .Build()) {}

AccessibilityLabelsServiceFactory::~AccessibilityLabelsServiceFactory() {}

KeyedService* AccessibilityLabelsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // ChromeOS creates various profiles (login, lock screen...) that do
  // not display web content and thus do not need the accessibility labels
  // service.
  if (!chromeos::ProfileHelper::IsRegularProfile(profile))
    return nullptr;
#endif

  return new AccessibilityLabelsService(profile);
}
