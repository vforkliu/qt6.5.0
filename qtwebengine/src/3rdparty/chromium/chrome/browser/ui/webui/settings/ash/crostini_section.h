// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_CROSTINI_SECTION_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_CROSTINI_SECTION_H_

#include "base/values.h"
#include "chrome/browser/ui/webui/settings/ash/os_settings_section.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chrome/browser/ui/webui/settings/ash/search/search_tag_registry.h"
#include "components/prefs/pref_change_registrar.h"

namespace content {
class WebUIDataSource;
}  // namespace content

namespace chromeos {
namespace settings {

// Provides UI strings and search tags for Crostini settings. Search tags are
// only added if Crostini is available, and subpage search tags are added only
// when those subpages are available.
class CrostiniSection : public OsSettingsSection {
 public:
  CrostiniSection(Profile* profile,
                  SearchTagRegistry* search_tag_registry,
                  PrefService* pref_service);
  ~CrostiniSection() override;

 private:
  // OsSettingsSection:
  void AddLoadTimeData(content::WebUIDataSource* html_source) override;
  void AddHandlers(content::WebUI* web_ui) override;
  int GetSectionNameMessageId() const override;
  mojom::Section GetSection() const override;
  ash::settings::mojom::SearchResultIcon GetSectionIcon() const override;
  std::string GetSectionPath() const override;
  bool LogMetric(mojom::Setting setting, base::Value& value) const override;
  void RegisterHierarchy(HierarchyGenerator* generator) const override;

  bool IsExportImportAllowed() const;
  bool IsContainerUpgradeAllowed() const;
  bool IsPortForwardingAllowed() const;
  bool IsMultiContainerAllowed() const;

  void UpdateSearchTags();

  PrefService* pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
  Profile* const profile_;
};

}  // namespace settings
}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when it moved to ash.
namespace ash::settings {
using ::chromeos::settings::CrostiniSection;
}

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_CROSTINI_SECTION_H_
