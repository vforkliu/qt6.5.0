// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_

#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/common/extensions/api/cryptotoken_private.h"
#include "extensions/browser/extension_function.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

// Implementations for chrome.cryptotokenPrivate API functions.

namespace extensions {
namespace api {

void CryptotokenRegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry);

class CryptotokenPrivateCanOriginAssertAppIdFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateCanOriginAssertAppIdFunction();
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.canOriginAssertAppId",
                             CRYPTOTOKENPRIVATE_CANORIGINASSERTAPPID)
 protected:
  ~CryptotokenPrivateCanOriginAssertAppIdFunction() override = default;
  ResponseAction Run() override;
};

class CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction();
  DECLARE_EXTENSION_FUNCTION(
      "cryptotokenPrivate.isAppIdHashInEnterpriseContext",
      CRYPTOTOKENPRIVATE_ISAPPIDHASHINENTERPRISECONTEXT)

 protected:
  ~CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction() override =
      default;
  ResponseAction Run() override;
};

class CryptotokenPrivateCanAppIdGetAttestationFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateCanAppIdGetAttestationFunction();
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.canAppIdGetAttestation",
                             CRYPTOTOKENPRIVATE_CANAPPIDGETATTESTATION)

 protected:
  ~CryptotokenPrivateCanAppIdGetAttestationFunction() override = default;
  ResponseAction Run() override;
  void Complete(bool result);
};

class CryptotokenPrivateCanMakeU2fApiRequestFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateCanMakeU2fApiRequestFunction();
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.canMakeU2fApiRequest",
                             CRYPTOTOKENPRIVATE_CANMAKEU2FAPIREQUEST)

 protected:
  ~CryptotokenPrivateCanMakeU2fApiRequestFunction() override = default;
  ResponseAction Run() override;
  void Complete(bool result);
};

class CryptotokenPrivateRecordRegisterRequestFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateRecordRegisterRequestFunction() = default;
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.recordRegisterRequest",
                             CRYPTOTOKENPRIVATE_RECORDREGISTERREQUEST)

 protected:
  ~CryptotokenPrivateRecordRegisterRequestFunction() override = default;
  ResponseAction Run() override;
};

class CryptotokenPrivateRecordSignRequestFunction : public ExtensionFunction {
 public:
  CryptotokenPrivateRecordSignRequestFunction() = default;
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.recordSignRequest",
                             CRYPTOTOKENPRIVATE_RECORDSIGNREQUEST)

 protected:
  ~CryptotokenPrivateRecordSignRequestFunction() override = default;
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_
