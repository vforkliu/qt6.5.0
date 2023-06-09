// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_SIGNALS_CONSTANTS_H_
#define COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_SIGNALS_CONSTANTS_H_

namespace device_signals {

// Signal names can be used as keys to store/retrieve signal values from
// dictionaries.
namespace names {

extern const char kAllowScreenLock[];
extern const char kBrowserVersion[];
extern const char kBuiltInDnsClientEnabled[];
extern const char kChromeCleanupEnabled[];
extern const char kChromeRemoteDesktopAppBlocked[];
extern const char kDeviceAffiliationIds[];
extern const char kDeviceHostName[];
extern const char kDeviceManufacturer[];
extern const char kDeviceModel[];
extern const char kDiskEncrypted[];
extern const char kDisplayName[];
extern const char kDeviceEnrollmentDomain[];
extern const char kOsFirewall[];
extern const char kImei[];
extern const char kMacAddresses[];
extern const char kMeid[];
extern const char kOs[];
extern const char kOsVersion[];
extern const char kPasswordProtectionWarningTrigger[];
extern const char kProfileAffiliationIds[];
extern const char kRealtimeUrlCheckMode[];
extern const char kSafeBrowsingProtectionLevel[];
extern const char kScreenLockSecured[];
extern const char kSecureBootEnabled[];
extern const char kSerialNumber[];
extern const char kSiteIsolationEnabled[];
extern const char kSystemDnsServers[];
extern const char kThirdPartyBlockingEnabled[];
extern const char kWindowsMachineDomain[];
extern const char kWindowsUserDomain[];

}  // namespace names

// Error strings that can be returned to indicated why signal values were not
// returned.
namespace errors {

extern const char kConsentRequired[];
extern const char kUnaffiliatedUser[];
extern const char kInvalidUser[];
extern const char kUnsupported[];
extern const char kMissingSystemService[];
extern const char kMissingBundle[];
extern const char kMissingParameters[];

}  // namespace errors

}  // namespace device_signals

#endif  // COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_SIGNALS_CONSTANTS_H_
