// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_page_handler.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/commerce/shopping_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "chrome/browser/ui/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils_desktop.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/commerce/shopping_list_context_menu_controller.h"
#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_side_panel_ui.h"
#include "chrome/browser/ui/webui/side_panel/reading_list/reading_list_ui.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/base/window_open_disposition.h"
#include "ui/base/window_open_disposition_utils.h"

namespace {

class BookmarkContextMenu : public ui::SimpleMenuModel,
                            public ui::SimpleMenuModel::Delegate,
                            public BookmarkContextMenuControllerDelegate {
 public:
  explicit BookmarkContextMenu(
      Browser* browser,
      base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder,
      const bookmarks::BookmarkNode* bookmark,
      const side_panel::mojom::ActionSource& source)
      : ui::SimpleMenuModel(this),
        embedder_(embedder),
        controller_(base::WrapUnique(new BookmarkContextMenuController(
            browser->window()->GetNativeWindow(),
            this,
            browser,
            browser->profile(),
            BookmarkLaunchLocation::kSidePanelContextMenu,
            bookmark->parent(),
            {bookmark}))),
        shopping_list_controller_(
            base::WrapUnique(new commerce::ShoppingListContextMenuController(
                BookmarkModelFactory::GetForBrowserContext(browser->profile()),
                commerce::ShoppingServiceFactory::GetForBrowserContext(
                    browser->profile()),
                bookmark,
                this))) {
    if (source == side_panel::mojom::ActionSource::kPriceTracking) {
      AddItem(IDC_BOOKMARK_BAR_OPEN_ALL);
      AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_NEW_WINDOW);
      AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_INCOGNITO);
      AddSeparator(ui::NORMAL_SEPARATOR);
      shopping_list_controller_->AddPriceTrackingItemForBookmark();
      AddSeparator(ui::NORMAL_SEPARATOR);
      AddItem(IDC_BOOKMARK_MANAGER);
      return;
    }

    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL);
    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_NEW_WINDOW);
    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_INCOGNITO);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(bookmark->is_folder() ? IDC_BOOKMARK_BAR_RENAME_FOLDER
                                  : IDC_BOOKMARK_BAR_EDIT);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_CUT);
    AddItem(IDC_COPY);
    AddItem(IDC_PASTE);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_BAR_REMOVE);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_BAR_ADD_NEW_BOOKMARK);
    AddItem(IDC_BOOKMARK_BAR_NEW_FOLDER);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_MANAGER);
  }
  ~BookmarkContextMenu() override = default;

  void ExecuteCommand(int command_id, int event_flags) override {
    if (shopping_list_controller_->ExecuteCommand(command_id))
      return;
    controller_->ExecuteCommand(command_id, event_flags);
  }

  bool IsCommandIdEnabled(int command_id) const override {
    return controller_->IsCommandIdEnabled(command_id);
  }

  bool IsCommandIdVisible(int command_id) const override {
    return controller_->IsCommandIdVisible(command_id);
  }

  // BookmarkContextMenuControllerDelegate:
  void CloseMenu() override { embedder_->HideContextMenu(); }

 private:
  void AddItem(int command_id) {
    ui::SimpleMenuModel::AddItem(command_id,
                                 controller_->menu_model()->GetLabelAt(
                                     controller_->menu_model()
                                         ->GetIndexOfCommandId(command_id)
                                         .value()));
  }
  base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder_;
  std::unique_ptr<BookmarkContextMenuController> controller_;
  std::unique_ptr<commerce::ShoppingListContextMenuController>
      shopping_list_controller_;
};

}  // namespace

BookmarksPageHandler::BookmarksPageHandler(
    mojo::PendingReceiver<side_panel::mojom::BookmarksPageHandler> receiver,
    BookmarksSidePanelUI* bookmarks_ui)
    : receiver_(this, std::move(receiver)), bookmarks_ui_(bookmarks_ui) {}

BookmarksPageHandler::BookmarksPageHandler(
    mojo::PendingReceiver<side_panel::mojom::BookmarksPageHandler> receiver,
    ReadingListUI* reading_list_ui)
    : receiver_(this, std::move(receiver)), reading_list_ui_(reading_list_ui) {}

BookmarksPageHandler::~BookmarksPageHandler() = default;

void BookmarksPageHandler::OpenBookmark(
    int64_t node_id,
    int32_t parent_folder_depth,
    ui::mojom::ClickModifiersPtr click_modifiers,
    side_panel::mojom::ActionSource source) {
  Browser* browser = chrome::FindLastActive();
  if (!browser)
    return;

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser->profile());
  const bookmarks::BookmarkNode* bookmark_node =
      bookmarks::GetBookmarkNodeByID(bookmark_model, node_id);
  if (!bookmark_node)
    return;

  WindowOpenDisposition open_location = ui::DispositionFromClick(
      click_modifiers->middle_button, click_modifiers->alt_key,
      click_modifiers->ctrl_key, click_modifiers->meta_key,
      click_modifiers->shift_key);
  chrome::OpenAllIfAllowed(browser, {bookmark_node}, open_location, false);
  if (source == side_panel::mojom::ActionSource::kPriceTracking)
    return;
  base::RecordAction(base::UserMetricsAction("SidePanel.Bookmarks.Navigation"));
  RecordBookmarkLaunch(
      parent_folder_depth > 0 ? BookmarkLaunchLocation::kSidePanelSubfolder
                              : BookmarkLaunchLocation::kSidePanelFolder,
      profile_metrics::GetBrowserProfileType(browser->profile()));
}

void BookmarksPageHandler::ShowContextMenu(
    const std::string& id_string,
    const gfx::Point& point,
    side_panel::mojom::ActionSource source) {
  int64_t id;
  if (!base::StringToInt64(id_string, &id))
    return;

  Browser* browser = chrome::FindLastActive();
  if (!browser)
    return;

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser->profile());
  const bookmarks::BookmarkNode* bookmark =
      bookmarks::GetBookmarkNodeByID(bookmark_model, id);
  if (!bookmark)
    return;

  auto embedder =
      bookmarks_ui_ ? bookmarks_ui_->embedder() : reading_list_ui_->embedder();
  if (embedder) {
    embedder->ShowContextMenu(point, std::make_unique<BookmarkContextMenu>(
                                         browser, embedder, bookmark, source));
  }
}

void BookmarksPageHandler::ShowUI() {
  auto embedder = bookmarks_ui_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}
