// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_APP_TYPES_H_
#define COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_APP_TYPES_H_

#include <string>
#include <utility>
#include <vector>

#include "base/component_export.h"
#include "base/time/time.h"
#include "components/services/app_service/public/cpp/icon_types.h"
#include "components/services/app_service/public/cpp/intent_filter.h"
#include "components/services/app_service/public/cpp/macros.h"
#include "components/services/app_service/public/cpp/permission.h"
#include "components/services/app_service/public/cpp/run_on_os_login_types.h"
#include "components/services/app_service/public/cpp/shortcut.h"
#include "components/services/app_service/public/mojom/types.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace apps {

// When updating the enum below, update
// //components/services/app_service/public/cpp/macros.h
// macros if necessary, as well as the ApplicationType enum in
// //components/services/app_service/public/protos/app_types.proto.
ENUM(AppType,
     kUnknown,
     kArc,                         // Android app.
     kBuiltIn,                     // Built-in app.
     kCrostini,                    // Linux (via Crostini) app.
     kChromeApp,                   // Chrome app.
     kWeb,                         // Web app.
     kMacOs,                       // Mac OS app.
     kPluginVm,                    // Plugin VM app, see go/pluginvm.
     kStandaloneBrowser,           // Lacros browser app, see //docs/lacros.md.
     kRemote,                      // Remote app.
     kBorealis,                    // Borealis app, see go/borealis-app.
     kSystemWeb,                   // System web app.
     kStandaloneBrowserChromeApp,  // Chrome app hosted in Lacros.
     kExtension,                   // Browser extension.
     kStandaloneBrowserExtension   // Extension hosted in Lacros.
)

// Whether an app is ready to launch, i.e. installed.
// Note the enumeration is used in UMA histogram so entries should not be
// re-ordered or removed. New entries should be added at the bottom.
ENUM(Readiness,
     kUnknown,
     kReady,                // Installed and launchable.
     kDisabledByBlocklist,  // Disabled by SafeBrowsing.
     kDisabledByPolicy,     // Disabled by admin policy.
     kDisabledByUser,       // Disabled by explicit user action.
     kTerminated,           // Renderer process crashed.
     kUninstalledByUser,
     // Removed apps are purged from the registry cache and have their
     // associated memory freed. Subscribers are not notified of removed
     // apps, so publishers must set the app as uninstalled before
     // removing it.
     kRemoved,
     kUninstalledByMigration)

// How the app was installed.
// This should be kept in sync with histograms.xml, InstallReason in
// enums.xml as well as ApplicationInstallReason in
// //components/services/app_service/public/protos/app_types.proto.
//
// Note the enumeration is used in UMA histogram so entries should not be
// re-ordered or removed. New entries should be added at the bottom.
ENUM(InstallReason,
     kUnknown,
     kSystem,   // Installed with the system and is considered a part of the OS.
     kPolicy,   // Installed by policy.
     kOem,      // Installed by an OEM.
     kDefault,  // Preinstalled by default, but is not considered a system app.
     kSync,     // Installed by sync.
     kUser,     // Installed by user action.
     kSubApp,   // Installed by the SubApp API call.
     kKiosk,    // Installed by Kiosk on Chrome OS.
     kCommandLine  // Installed by command line argument.
)

// Where the app was installed from.
// This should be kept in sync with histograms.xml, InstallSource in
// enums.xml as well as ApplicationInstallSource in
// //components/services/app_service/public/protos/app_types.proto.
//
// Note the enumeration is used in UMA histogram so entries should not be
// re-ordered or removed. New entries should be added at the bottom.
ENUM(InstallSource,
     kUnknown,
     kSystem,          // Installed as part of Chrome OS.
     kSync,            // Installed from sync.
     kPlayStore,       // Installed from Play store.
     kChromeWebStore,  // Installed from Chrome web store.
     kBrowser          // Installed from browser.
)

// What caused the app to be uninstalled.
// This should be kept in sync with UninstallSource in enums.xml as well as
// ApplicationUninstallSource in
// //components/services/app_service/public/protos/app_types.proto, so entries
// should not be re-ordered or removed. New entries should be added at the
// bottom.
ENUM(UninstallSource,
     kUnknown,
     kAppList,        // Uninstall by the user from the App List (Launcher)
     kAppManagement,  // Uninstall by the user from the App Management page
     kShelf,          // Uninstall by the user from the Shelf
     kMigration       // Uninstall by app migration.
)

// The window mode that each app will open in.
ENUM(WindowMode,
     kUnknown,
     // Opens in a standalone window
     kWindow,
     // Opens in the default web browser
     kBrowser,
     // Opens in a tabbed app window
     kTabbedWindow)

// Information about an app. See components/services/app_service/README.md.
struct COMPONENT_EXPORT(APP_TYPES) App {
  App(AppType app_type, const std::string& app_id);

  App(const App&) = delete;
  App& operator=(const App&) = delete;

  ~App();

  std::unique_ptr<App> Clone() const;

  AppType app_type;
  std::string app_id;

  Readiness readiness = Readiness::kUnknown;
  absl::optional<std::string> name;
  absl::optional<std::string> short_name;

  // An optional, publisher-specific ID for this app, e.g. for Android apps,
  // this field contains the Android package name, and for web apps, it
  // contains the start URL.
  absl::optional<std::string> publisher_id;

  absl::optional<std::string> description;
  absl::optional<std::string> version;
  std::vector<std::string> additional_search_terms;

  absl::optional<IconKey> icon_key;

  absl::optional<base::Time> last_launch_time;
  absl::optional<base::Time> install_time;

  // This vector must be treated atomically, if there is a permission
  // change, the publisher must send through the entire list of permissions.
  // Should contain no duplicate IDs.
  // If empty during updates, Subscriber can assume no changes.
  // There is no guarantee that this is sorted by any criteria.
  Permissions permissions;

  // Whether the app was installed by sync, policy or as a default app.
  InstallReason install_reason = InstallReason::kUnknown;

  // Where the app was installed from, e.g. from Play Store, from Chrome Web
  // Store, etc.
  InstallSource install_source = InstallSource::kUnknown;

  // IDs used for policy to identify the app.
  // For web apps, it contains the install URL(s).
  std::vector<std::string> policy_ids;

  // Whether the app is an extensions::Extensions where is_platform_app()
  // returns true.
  absl::optional<bool> is_platform_app;

  absl::optional<bool> recommendable;
  absl::optional<bool> searchable;
  absl::optional<bool> show_in_launcher;
  absl::optional<bool> show_in_shelf;
  absl::optional<bool> show_in_search;
  absl::optional<bool> show_in_management;

  // True if the app is able to handle intents and should be shown in intent
  // surfaces.
  absl::optional<bool> handles_intents;

  // Whether the app publisher allows the app to be uninstalled.
  absl::optional<bool> allow_uninstall;

  // Whether the app icon should add the notification badging.
  absl::optional<bool> has_badge;

  // Paused apps cannot be launched, and any running apps that become paused
  // will be stopped. This is independent of whether or not the app is ready to
  // be launched (defined by the Readiness field).
  absl::optional<bool> paused;

  // This vector stores all the intent filters defined in this app. Each
  // intent filter defines a matching criteria for whether an intent can
  // be handled by this app. One app can have multiple intent filters.
  IntentFilters intent_filters;

  // Whether the app can be free resized. If this is true, various resizing
  // operations will be restricted.
  absl::optional<bool> resize_locked;

  // Whether the app's display mode is in the browser or otherwise.
  WindowMode window_mode = WindowMode::kUnknown;

  // Whether the app runs on os login in a new window or not.
  absl::optional<RunOnOsLogin> run_on_os_login;

  // Shortcuts help users perform specific actions easily.
  // This vector must be treated atomically, if there is a shortcut
  // change, the publisher must send through the entire list of shortcuts.
  // Should contain no duplicate IDs.
  // If empty during updates, Subscriber can assume no changes.
  // There is no guarantee that this is sorted by any criteria.
  Shortcuts shortcuts;

  // Storage space size for app and associated data.
  absl::optional<uint64_t> app_size_in_bytes;
  absl::optional<uint64_t> data_size_in_bytes;

  // When adding new fields to the App type, the `Clone` function and the
  // `AppUpdate` class should also be updated.
};

using AppPtr = std::unique_ptr<App>;

// TODO(crbug.com/1253250): Remove these functions after migrating to non-mojo
// AppService.
COMPONENT_EXPORT(APP_TYPES)
AppType ConvertMojomAppTypToAppType(apps::mojom::AppType mojom_app_type);

COMPONENT_EXPORT(APP_TYPES)
mojom::AppType ConvertAppTypeToMojomAppType(AppType app_type);

COMPONENT_EXPORT(APP_TYPES)
Readiness ConvertMojomReadinessToReadiness(
    apps::mojom::Readiness mojom_readiness);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::Readiness ConvertReadinessToMojomReadiness(Readiness readiness);

COMPONENT_EXPORT(APP_TYPES)
InstallReason ConvertMojomInstallReasonToInstallReason(
    apps::mojom::InstallReason mojom_install_reason);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::InstallReason ConvertInstallReasonToMojomInstallReason(
    InstallReason install_reason);

COMPONENT_EXPORT(APP_TYPES)
InstallSource ConvertMojomInstallSourceToInstallSource(
    apps::mojom::InstallSource mojom_install_source);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::InstallSource ConvertInstallSourceToMojomInstallSource(
    InstallSource install_source);

COMPONENT_EXPORT(APP_TYPES)
WindowMode ConvertMojomWindowModeToWindowMode(
    apps::mojom::WindowMode mojom_window_mode);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::WindowMode ConvertWindowModeToMojomWindowMode(
    WindowMode mojom_window_mode);

COMPONENT_EXPORT(APP_TYPES)
absl::optional<bool> GetOptionalBool(
    const apps::mojom::OptionalBool& mojom_optional_bool);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::OptionalBool GetMojomOptionalBool(
    const absl::optional<bool>& mojom_optional_bool);

COMPONENT_EXPORT(APP_TYPES)
AppPtr ConvertMojomAppToApp(const apps::mojom::AppPtr& mojom_app);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::AppPtr ConvertAppToMojomApp(const AppPtr& app);

COMPONENT_EXPORT(APP_TYPES)
std::vector<base::FilePath> ConvertMojomFilePathsToFilePaths(
    apps::mojom::FilePathsPtr mojom_file_paths);

COMPONENT_EXPORT(APP_TYPES)
UninstallSource ConvertMojomUninstallSourceToUninstallSource(
    apps::mojom::UninstallSource mojom_uninstall_source);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::UninstallSource ConvertUninstallSourceToMojomUninstallSource(
    UninstallSource uninstall_source);

COMPONENT_EXPORT(APP_TYPES)
apps::mojom::UninstallSource ConvertUninstallSourceToMojomUninstallSource(
    UninstallSource uninstall_source);

}  // namespace apps

#endif  // COMPONENTS_SERVICES_APP_SERVICE_PUBLIC_CPP_APP_TYPES_H_
