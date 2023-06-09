// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/bookmark_manager_private/bookmark_manager_private_api.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/url_and_id.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_constants.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_helpers.h"
#include "chrome/browser/extensions/api/tabs/windows_util.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/ui/bookmarks/bookmark_drag_drop.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/browser/undo/bookmark_undo_service_factory.h"
#include "chrome/common/extensions/api/bookmark_manager_private.h"
#include "chrome/common/extensions/extension_constants.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/scoped_group_bookmark_actions.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/mojom/view_type.mojom.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-shared.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::BookmarkNodeData;
using content::WebContents;

namespace extensions {

namespace bookmark_keys = bookmark_api_constants;
namespace bookmark_manager_private = api::bookmark_manager_private;
namespace CanPaste = api::bookmark_manager_private::CanPaste;
namespace Copy = api::bookmark_manager_private::Copy;
namespace Cut = api::bookmark_manager_private::Cut;
namespace Drop = api::bookmark_manager_private::Drop;
namespace GetSubtree = api::bookmark_manager_private::GetSubtree;
namespace Paste = api::bookmark_manager_private::Paste;
namespace RemoveTrees = api::bookmark_manager_private::RemoveTrees;
namespace SortChildren = api::bookmark_manager_private::SortChildren;
namespace StartDrag = api::bookmark_manager_private::StartDrag;
namespace OpenInNewTab = api::bookmark_manager_private::OpenInNewTab;
namespace OpenInNewWindow = api::bookmark_manager_private::OpenInNewWindow;

namespace {

// Returns a single bookmark node from the argument ID.
// This returns nullptr in case of failure.
const BookmarkNode* GetNodeFromString(BookmarkModel* model,
                                      const std::string& id_string) {
  int64_t id;
  if (!base::StringToInt64(id_string, &id))
    return nullptr;
  return bookmarks::GetBookmarkNodeByID(model, id);
}

// Gets a vector of bookmark nodes from the argument list of IDs.
// This returns false in the case of failure.
bool GetNodesFromVector(BookmarkModel* model,
                        const std::vector<std::string>& id_strings,
                        std::vector<const BookmarkNode*>* nodes) {
  if (id_strings.empty())
    return false;

  for (size_t i = 0; i < id_strings.size(); ++i) {
    const BookmarkNode* node = GetNodeFromString(model, id_strings[i]);
    if (!node)
      return false;
    nodes->push_back(node);
  }

  return true;
}

// Recursively create a bookmark_manager_private::BookmarkNodeDataElement from
// a bookmark node. This is by used |BookmarkNodeDataToJSON| when the data comes
// from the current profile. In this case we have a BookmarkNode since we got
// the data from the current profile.
bookmark_manager_private::BookmarkNodeDataElement
CreateNodeDataElementFromBookmarkNode(const BookmarkNode& node) {
  bookmark_manager_private::BookmarkNodeDataElement element;
  // Add id and parentId so we can associate the data with existing nodes on the
  // client side.
  element.id = base::NumberToString(node.id());
  element.parent_id = base::NumberToString(node.parent()->id());

  if (node.is_url())
    element.url = node.url().spec();

  element.title = base::UTF16ToUTF8(node.GetTitle());
  for (const auto& child : node.children()) {
    element.children.push_back(CreateNodeDataElementFromBookmarkNode(*child));
  }

  return element;
}

// Recursively create a bookmark_manager_private::BookmarkNodeDataElement from
// a BookmarkNodeData::Element. This is used by |BookmarkNodeDataToJSON| when
// the data comes from a different profile. When the data comes from a different
// profile we do not have any IDs or parent IDs.
bookmark_manager_private::BookmarkNodeDataElement CreateApiNodeDataElement(
    const BookmarkNodeData::Element& element) {
  bookmark_manager_private::BookmarkNodeDataElement node_element;

  if (element.is_url)
    node_element.url = element.url.spec();
  node_element.title = base::UTF16ToUTF8(element.title);
  for (size_t i = 0; i < element.children.size(); ++i) {
    node_element.children.push_back(
        CreateApiNodeDataElement(element.children[i]));
  }

  return node_element;
}

// Creates a bookmark_manager_private::BookmarkNodeData from a BookmarkNodeData.
bookmark_manager_private::BookmarkNodeData CreateApiBookmarkNodeData(
    Profile* profile,
    const BookmarkNodeData& data) {
  const base::FilePath& profile_path = profile->GetPath();

  bookmark_manager_private::BookmarkNodeData node_data;
  node_data.same_profile = data.IsFromProfilePath(profile_path);

  if (node_data.same_profile) {
    std::vector<const BookmarkNode*> nodes = data.GetNodes(
        BookmarkModelFactory::GetForBrowserContext(profile), profile_path);
    for (size_t i = 0; i < nodes.size(); ++i) {
      node_data.elements.push_back(
          CreateNodeDataElementFromBookmarkNode(*nodes[i]));
    }
  } else {
    // We do not have a node IDs when the data comes from a different profile.
    for (size_t i = 0; i < data.size(); ++i)
      node_data.elements.push_back(CreateApiNodeDataElement(data.elements[i]));
  }
  return node_data;
}

bool HasPermanentNodes(const std::vector<const BookmarkNode*>& list) {
  for (const BookmarkNode* node : list) {
    if (node->is_permanent_node())
      return true;
  }
  return false;
}

}  // namespace

BookmarkManagerPrivateEventRouter::BookmarkManagerPrivateEventRouter(
    content::BrowserContext* browser_context,
    BookmarkModel* bookmark_model)
    : browser_context_(browser_context), bookmark_model_(bookmark_model) {
  bookmark_model_->AddObserver(this);
}

BookmarkManagerPrivateEventRouter::~BookmarkManagerPrivateEventRouter() {
  if (bookmark_model_)
    bookmark_model_->RemoveObserver(this);
}

void BookmarkManagerPrivateEventRouter::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    base::Value::List event_args) {
  EventRouter::Get(browser_context_)
      ->BroadcastEvent(std::make_unique<Event>(histogram_value, event_name,
                                               std::move(event_args)));
}

void BookmarkManagerPrivateEventRouter::BookmarkModelChanged() {}

void BookmarkManagerPrivateEventRouter::BookmarkModelBeingDeleted(
    BookmarkModel* model) {
  bookmark_model_ = nullptr;
}

BookmarkManagerPrivateAPI::BookmarkManagerPrivateAPI(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
}

BookmarkManagerPrivateAPI::~BookmarkManagerPrivateAPI() = default;

void BookmarkManagerPrivateAPI::Shutdown() {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<BookmarkManagerPrivateAPI>>::DestructorAtExit
    g_bookmark_manager_private_api_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<BookmarkManagerPrivateAPI>*
BookmarkManagerPrivateAPI::GetFactoryInstance() {
  return g_bookmark_manager_private_api_factory.Pointer();
}

void BookmarkManagerPrivateAPI::OnListenerAdded(
    const EventListenerInfo& details) {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
  event_router_ = std::make_unique<BookmarkManagerPrivateEventRouter>(
      browser_context_,
      BookmarkModelFactory::GetForBrowserContext(browser_context_));
}

BookmarkManagerPrivateDragEventRouter::BookmarkManagerPrivateDragEventRouter(
    content::WebContents* web_contents)
    : content::WebContentsUserData<BookmarkManagerPrivateDragEventRouter>(
          *web_contents),
      profile_(Profile::FromBrowserContext(web_contents->GetBrowserContext())) {
  // We need to guarantee the BookmarkTabHelper is created.
  BookmarkTabHelper::CreateForWebContents(web_contents);
  BookmarkTabHelper* bookmark_tab_helper =
      BookmarkTabHelper::FromWebContents(web_contents);
  bookmark_tab_helper->set_bookmark_drag_delegate(this);
}

BookmarkManagerPrivateDragEventRouter::
    ~BookmarkManagerPrivateDragEventRouter() {
  // No need to remove ourselves as the BookmarkTabHelper's delegate, since they
  // are both WebContentsUserData and will be deleted at the same time.
}

void BookmarkManagerPrivateDragEventRouter::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    base::Value::List args) {
  EventRouter* event_router = EventRouter::Get(profile_);
  if (!event_router)
    return;

  std::unique_ptr<Event> event(
      new Event(histogram_value, event_name, std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void BookmarkManagerPrivateDragEventRouter::OnDragEnter(
    const BookmarkNodeData& data) {
  if (!data.is_valid())
    return;
  DispatchEvent(events::BOOKMARK_MANAGER_PRIVATE_ON_DRAG_ENTER,
                bookmark_manager_private::OnDragEnter::kEventName,
                bookmark_manager_private::OnDragEnter::Create(
                    CreateApiBookmarkNodeData(profile_, data)));
}

void BookmarkManagerPrivateDragEventRouter::OnDragOver(
    const BookmarkNodeData& data) {
  // Intentionally empty since these events happens too often and floods the
  // message queue. We do not need this event for the bookmark manager anyway.
}

void BookmarkManagerPrivateDragEventRouter::OnDragLeave(
    const BookmarkNodeData& data) {
  if (!data.is_valid())
    return;
  DispatchEvent(events::BOOKMARK_MANAGER_PRIVATE_ON_DRAG_LEAVE,
                bookmark_manager_private::OnDragLeave::kEventName,
                bookmark_manager_private::OnDragLeave::Create(
                    CreateApiBookmarkNodeData(profile_, data)));
}

void BookmarkManagerPrivateDragEventRouter::OnDrop(
    const BookmarkNodeData& data) {
  if (!data.is_valid())
    return;
  DispatchEvent(events::BOOKMARK_MANAGER_PRIVATE_ON_DROP,
                bookmark_manager_private::OnDrop::kEventName,
                bookmark_manager_private::OnDrop::Create(
                    CreateApiBookmarkNodeData(profile_, data)));

  // Make a copy that is owned by this instance.
  ClearBookmarkNodeData();
  bookmark_drag_data_ = data;
}

const BookmarkNodeData*
BookmarkManagerPrivateDragEventRouter::GetBookmarkNodeData() {
  if (bookmark_drag_data_.is_valid())
    return &bookmark_drag_data_;
  return nullptr;
}

void BookmarkManagerPrivateDragEventRouter::ClearBookmarkNodeData() {
  bookmark_drag_data_.Clear();
}

ExtensionFunction::ResponseValue ClipboardBookmarkManagerFunction::CopyOrCut(
    bool cut,
    const std::vector<std::string>& id_list) {
  BookmarkModel* model = GetBookmarkModel();
  std::vector<const BookmarkNode*> nodes;
  if (!GetNodesFromVector(model, id_list, &nodes)) {
    return Error(bookmark_keys::kBookmarkNodesNotFoundFromIdListError,
                 base::JoinString(id_list, ", "));
  }

  bookmarks::ManagedBookmarkService* managed = GetManagedBookmarkService();
  if (cut && bookmarks::HasDescendantsOf(nodes, managed->managed_node()))
    return Error(bookmark_keys::kModifyManagedError);
  if (cut && HasPermanentNodes(nodes))
    return Error(bookmark_keys::kModifySpecialError);
  bookmarks::CopyToClipboard(model, nodes, cut);
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateCopyFunction::RunOnReady() {
  std::unique_ptr<Copy::Params> params(Copy::Params::Create(args()));
  if (!params)
    return BadMessage();
  return CopyOrCut(false, params->id_list);
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateCutFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  std::unique_ptr<Cut::Params> params(Cut::Params::Create(args()));
  if (!params)
    return BadMessage();
  return CopyOrCut(true, params->id_list);
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivatePasteFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  std::unique_ptr<Paste::Params> params(Paste::Params::Create(args()));
  if (!params)
    return BadMessage();
  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(GetProfile());
  const BookmarkNode* parent_node = GetNodeFromString(model, params->parent_id);
  std::string error;
  if (!CanBeModified(parent_node, &error))
    return Error(error);
  bool can_paste = bookmarks::CanPasteFromClipboard(model, parent_node);
  if (!can_paste)
    return Error("Could not paste from clipboard");

  // We want to use the highest index of the selected nodes as a destination.
  std::vector<const BookmarkNode*> nodes;
  // No need to test return value, if we got an empty list, we insert at end.
  if (params->selected_id_list)
    GetNodesFromVector(model, *params->selected_id_list, &nodes);
  size_t highest_index = 0;
  for (const BookmarkNode* node : nodes) {
    // + 1 so that we insert after the selection.
    highest_index =
        std::max(highest_index, parent_node->GetIndexOf(node).value() + 1);
  }
  if (!highest_index)
    highest_index = parent_node->children().size();

  bookmarks::PasteFromClipboard(model, parent_node, highest_index);
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateCanPasteFunction::RunOnReady() {
  std::unique_ptr<CanPaste::Params> params(CanPaste::Params::Create(args()));
  if (!params)
    return BadMessage();

  PrefService* prefs = user_prefs::UserPrefs::Get(GetProfile());
  if (!prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled))
    return WithArguments(false);

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(GetProfile());
  const BookmarkNode* parent_node = GetNodeFromString(model, params->parent_id);
  if (!parent_node)
    return Error(bookmark_keys::kNoParentError);
  bool can_paste = bookmarks::CanPasteFromClipboard(model, parent_node);
  return WithArguments(can_paste);
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateSortChildrenFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  std::unique_ptr<SortChildren::Params> params(
      SortChildren::Params::Create(args()));
  if (!params)
    return BadMessage();

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(GetProfile());
  const BookmarkNode* parent_node = GetNodeFromString(model, params->parent_id);
  std::string error;
  if (!CanBeModified(parent_node, &error))
    return Error(error);
  model->SortChildren(parent_node);
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateStartDragFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  content::WebContents* web_contents = GetSenderWebContents();
  std::unique_ptr<StartDrag::Params> params(StartDrag::Params::Create(args()));
  if (!params)
    return BadMessage();

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(GetProfile());
  std::vector<const BookmarkNode*> nodes;
  if (!GetNodesFromVector(model, params->id_list, &nodes)) {
    return Error(bookmark_keys::kBookmarkNodesNotFoundFromIdListError,
                 base::JoinString(params->id_list, ", "));
  }

  ui::mojom::DragEventSource source = ui::mojom::DragEventSource::kMouse;
  if (params->is_from_touch)
    source = ui::mojom::DragEventSource::kTouch;

  chrome::DragBookmarks(
      GetProfile(), {std::move(nodes), params->drag_node_index, web_contents,
                     source, gfx::Point(params->x, params->y)});

  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateDropFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  std::unique_ptr<Drop::Params> params(Drop::Params::Create(args()));
  if (!params)
    return BadMessage();

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(GetProfile());

  const BookmarkNode* drop_parent = GetNodeFromString(model, params->parent_id);
  std::string error;
  if (!CanBeModified(drop_parent, &error))
    return Error(error);

  content::WebContents* web_contents = GetSenderWebContents();
  size_t drop_index;
  if (params->index) {
    drop_index = static_cast<size_t>(*params->index);
    CHECK(drop_index >= 0 && drop_index <= drop_parent->children().size());
  } else {
    drop_index = drop_parent->children().size();
  }

  BookmarkManagerPrivateDragEventRouter* router =
      BookmarkManagerPrivateDragEventRouter::FromWebContents(web_contents);

  const BookmarkNodeData* drag_data = router->GetBookmarkNodeData();
  CHECK_NE(nullptr, drag_data) << "Somehow we're dropping null bookmark data";
  const bool copy = false;
  chrome::DropBookmarks(
      GetProfile(), *drag_data, drop_parent, drop_index, copy);

  router->ClearBookmarkNodeData();
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateGetSubtreeFunction::RunOnReady() {
  std::unique_ptr<GetSubtree::Params> params(
      GetSubtree::Params::Create(args()));
  if (!params)
    return BadMessage();

  const BookmarkNode* node = nullptr;

  if (params->id.empty()) {
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(GetProfile());
    node = model->root_node();
  } else {
    std::string error;
    node = GetBookmarkNodeFromId(params->id, &error);
    if (!node)
      return Error(error);
  }

  std::vector<api::bookmarks::BookmarkTreeNode> nodes;
  bookmarks::ManagedBookmarkService* managed = GetManagedBookmarkService();
  if (params->folders_only)
    bookmark_api_helpers::AddNodeFoldersOnly(managed, node, &nodes, true);
  else
    bookmark_api_helpers::AddNode(managed, node, &nodes, true);
  return ArgumentList(GetSubtree::Results::Create(nodes));
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateRemoveTreesFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  std::unique_ptr<RemoveTrees::Params> params(
      RemoveTrees::Params::Create(args()));
  if (!params)
    return BadMessage();

  BookmarkModel* model = GetBookmarkModel();
  bookmarks::ManagedBookmarkService* managed = GetManagedBookmarkService();
  bookmarks::ScopedGroupBookmarkActions group_deletes(model);
  int64_t id;
  std::string error;
  for (const std::string& id_string : params->id_list) {
    if (!base::StringToInt64(id_string, &id))
      return Error(bookmark_keys::kInvalidIdError);
    if (!bookmark_api_helpers::RemoveNode(model, managed, id, true, &error))
      return Error(error);
  }

  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateUndoFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  BookmarkUndoServiceFactory::GetForProfile(GetProfile())->undo_manager()->
      Undo();
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateRedoFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return Error(bookmark_keys::kEditBookmarksDisabled);

  BookmarkUndoServiceFactory::GetForProfile(GetProfile())->undo_manager()->
      Redo();
  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateOpenInNewTabFunction::RunOnReady() {
  std::unique_ptr<OpenInNewTab::Params> params(
      OpenInNewTab::Params::Create(args()));
  if (!params)
    return BadMessage();

  std::string error;
  const BookmarkNode* node = GetBookmarkNodeFromId(params->id, &error);
  if (!node)
    return Error(error);
  if (!node->is_url())
    return Error("Cannot open a folder in a new tab.");

  ExtensionTabUtil::OpenTabParams options;
  options.url = node->url().spec();
  options.active = params->active;
  options.bookmark_id = node->id();

  auto result =
      extensions::ExtensionTabUtil::OpenTab(this, options, user_gesture());
  if (!result.has_value())
    return Error(result.error());

  return WithArguments();
}

ExtensionFunction::ResponseValue
BookmarkManagerPrivateOpenInNewWindowFunction::RunOnReady() {
  std::unique_ptr<OpenInNewWindow::Params> params(
      OpenInNewWindow::Params::Create(args()));
  if (!params)
    return BadMessage();

  Profile* calling_profile = Profile::FromBrowserContext(browser_context());

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(calling_profile);
  std::vector<const BookmarkNode*> nodes;
  if (!GetNodesFromVector(model, params->id_list, &nodes)) {
    return Error(bookmark_keys::kBookmarkNodesNotFoundFromIdListError,
                 base::JoinString(params->id_list, ", "));
  }

  std::vector<GURL> urls;
  urls.reserve(nodes.size());
  for (const auto* node : nodes) {
    if (!node->is_url())
      return Error("Cannot open a folder in a new window.");
    urls.push_back(node->url());
  }

  std::string error;
  windows_util::IncognitoResult incognito_result =
      windows_util::ShouldOpenIncognitoWindow(calling_profile,
                                              params->incognito, &urls, &error);
  if (incognito_result == windows_util::IncognitoResult::kError)
    return Error(std::move(error));

  std::vector<UrlAndId> url_and_ids;
  urls.reserve(nodes.size());
  for (const auto* node : nodes) {
    if (!base::Contains(urls, node->url()))
      continue;  // The URL was filtered out; ignore this node.
    UrlAndId url_and_id;
    url_and_id.url = node->url();
    url_and_id.id = node->id();
    url_and_ids.push_back(url_and_id);
  }
  DCHECK_EQ(urls.size(), url_and_ids.size());

  DCHECK(!calling_profile->IsOffTheRecord());
  Profile* window_profile =
      incognito_result == windows_util::IncognitoResult::kIncognito
          ? calling_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true)
          : calling_profile;

  bool first_tab = true;
  for (auto& url_and_id : url_and_ids) {
    NavigateParams navigate_params(window_profile, url_and_id.url,
                                   ui::PAGE_TRANSITION_LINK);
    navigate_params.window_action = NavigateParams::WindowAction::SHOW_WINDOW;
    navigate_params.disposition =
        first_tab ? WindowOpenDisposition::NEW_WINDOW
                  : WindowOpenDisposition::NEW_FOREGROUND_TAB;
    if (params->incognito)
      navigate_params.disposition = WindowOpenDisposition::OFF_THE_RECORD;
    base::WeakPtr<content::NavigationHandle> handle =
        Navigate(&navigate_params);
    if (handle) {
      ChromeNavigationUIData* ui_data =
          static_cast<ChromeNavigationUIData*>(handle->GetNavigationUIData());
      ui_data->set_bookmark_id(url_and_id.id);
    }

    first_tab = false;
  }

  return WithArguments();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BookmarkManagerPrivateDragEventRouter);

}  // namespace extensions
