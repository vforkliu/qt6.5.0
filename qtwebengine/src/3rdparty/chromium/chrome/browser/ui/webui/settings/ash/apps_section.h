// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_APPS_SECTION_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_APPS_SECTION_H_

#include "ash/public/cpp/message_center_ash.h"
#include "base/values.h"
#include "chrome/browser/apps/app_service/app_service_proxy_forward.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/webui/settings/ash/os_settings_section.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chrome/browser/ui/webui/settings/ash/search/search_tag_registry.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class WebUIDataSource;
}  // namespace content

namespace chromeos {
namespace settings {

// Provides UI strings and search tags for Apps settings.
class AppsSection : public OsSettingsSection,
                    public ArcAppListPrefs::Observer,
                    public ash::MessageCenterAsh::Observer {
 public:
  AppsSection(Profile* profile,
              SearchTagRegistry* search_tag_registry,
              PrefService* pref_service,
              ArcAppListPrefs* arc_app_list_prefs,
              apps::AppServiceProxy* app_service_proxy);
  ~AppsSection() override;

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

  // ArcAppListPrefs::Observer:
  void OnAppRegistered(const std::string& app_id,
                       const ArcAppListPrefs::AppInfo& app_info) override;

  // MessageCenterAsh::Observer override:
  void OnQuietModeChanged(bool in_quiet_mode) override;

  void AddAndroidAppStrings(content::WebUIDataSource* html_source);
  void AddPluginVmLoadTimeData(content::WebUIDataSource* html_source);
  void AddOnStartupTimeData(content::WebUIDataSource* html_source);

  void UpdateAndroidSearchTags();

  PrefService* pref_service_;
  ArcAppListPrefs* arc_app_list_prefs_;
  apps::AppServiceProxy* app_service_proxy_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace settings
}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when it moved to ash.
namespace ash::settings {
using ::chromeos::settings::AppsSection;
}

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_ASH_APPS_SECTION_H_
