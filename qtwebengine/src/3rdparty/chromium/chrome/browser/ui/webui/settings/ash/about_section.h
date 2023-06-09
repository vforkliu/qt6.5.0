// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_ABOUT_SECTION_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_ABOUT_SECTION_H_

#include "base/values.h"
#include "build/branding_buildflags.h"
#include "chrome/browser/ui/webui/settings/ash/os_settings_section.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chrome/browser/ui/webui/settings/ash/search/search_tag_registry.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/user_manager/user_manager.h"

namespace content {
class WebUIDataSource;
}  // namespace content

namespace chromeos {
namespace settings {

// Provides UI strings and search tags for the settings "About Chrome OS" page.
class AboutSection : public OsSettingsSection {
 public:
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  AboutSection(Profile* profile,
               SearchTagRegistry* search_tag_registry,
               PrefService* pref_service);
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
  AboutSection(Profile* profile, SearchTagRegistry* search_tag_registry);
  ~AboutSection() override;

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

  // Returns if the auto update toggle should be shown for the active user.
  bool ShouldShowAUToggle(user_manager::User* active_user);

#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  void UpdateReportIssueSearchTags();

  PrefService* pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
};

}  // namespace settings
}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when it moved to ash.
namespace ash::settings {
using ::chromeos::settings::AboutSection;
}

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_ABOUT_SECTION_H_
