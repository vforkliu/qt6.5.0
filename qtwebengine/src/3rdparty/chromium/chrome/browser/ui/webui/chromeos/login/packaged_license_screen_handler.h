// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_PACKAGED_LICENSE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_PACKAGED_LICENSE_SCREEN_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class PackagedLicenseView : public base::SupportsWeakPtr<PackagedLicenseView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{"packaged-license",
                                                       "PackagedLicenseScreen"};

  PackagedLicenseView() = default;
  PackagedLicenseView(const PackagedLicenseView&) = delete;
  PackagedLicenseView& operator=(const PackagedLicenseView&) = delete;
  virtual ~PackagedLicenseView() = default;

  // Shows the contents of the screen.
  virtual void Show() = 0;
};

// A class that handles WebUI hooks in PackagedLicense screen.
class PackagedLicenseScreenHandler : public BaseScreenHandler,
                                     public PackagedLicenseView {
 public:
  using TView = PackagedLicenseView;
  PackagedLicenseScreenHandler();
  PackagedLicenseScreenHandler(const PackagedLicenseScreenHandler&) = delete;
  PackagedLicenseScreenHandler& operator=(const PackagedLicenseScreenHandler&) =
      delete;
  ~PackagedLicenseScreenHandler() override;

  void Show() override;

 private:
  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::PackagedLicenseScreenHandler;
using ::chromeos::PackagedLicenseView;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_PACKAGED_LICENSE_SCREEN_HANDLER_H_
