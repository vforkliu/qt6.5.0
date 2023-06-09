// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/offscreen/offscreen_api.h"

#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/offscreen/offscreen_document_manager.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/offscreen_document_host.h"
#include "extensions/common/api/offscreen.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace extensions {

namespace {

// Returns the BrowserContext with which offscreen documents should be
// associated for the given `extension` and `calling_context`. This may be
// different from the `calling_context`, as in the case of spanning mode
// extensions.
content::BrowserContext& GetBrowserContextToUse(
    content::BrowserContext& calling_context,
    const Extension& extension) {
  // The on-the-record profile always uses itself.
  if (!calling_context.IsOffTheRecord())
    return calling_context;

  DCHECK(util::IsIncognitoEnabled(extension.id(), &calling_context))
      << "Only incognito-enabled extensions should have an incognito context";

  // Split-mode extensions use the incognito (calling) context; spanning mode
  // extensions fall back to the original profile.
  bool is_split_mode = IncognitoInfo::IsSplitMode(&extension);
  return is_split_mode ? calling_context
                       : *ExtensionsBrowserClient::Get()->GetOriginalContext(
                             &calling_context);
}

// Similar to the above, returns the OffscreenDocumentManager to use for the
// given `extension` and `calling_context`.
OffscreenDocumentManager* GetManagerToUse(
    content::BrowserContext& calling_context,
    const Extension& extension) {
  return OffscreenDocumentManager::Get(
      &GetBrowserContextToUse(calling_context, extension));
}

}  // namespace

OffscreenCreateDocumentFunction::OffscreenCreateDocumentFunction() = default;
OffscreenCreateDocumentFunction::~OffscreenCreateDocumentFunction() = default;

ExtensionFunction::ResponseAction OffscreenCreateDocumentFunction::Run() {
  std::unique_ptr<api::offscreen::CreateDocument::Params> params(
      api::offscreen::CreateDocument::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  EXTENSION_FUNCTION_VALIDATE(extension());

  GURL url(params->parameters.url);
  if (!url.is_valid())
    url = extension()->GetResourceURL(params->parameters.url);

  if (!url.is_valid() || url::Origin::Create(url) != extension()->origin()) {
    return RespondNow(Error("Invalid URL."));
  }

  OffscreenDocumentManager* manager =
      GetManagerToUse(*browser_context(), *extension());

  if (manager->GetOffscreenDocumentForExtension(*extension())) {
    return RespondNow(
        Error("Only a single offscreen document may be created."));
  }

  OffscreenDocumentHost* offscreen_document =
      manager->CreateOffscreenDocument(*extension(), url);
  DCHECK(offscreen_document);

  // We assume it's impossible for a document to entirely synchronously load. If
  // that ever changes, we'll need to update this to check the status of the
  // load and respond synchronously.
  DCHECK(!offscreen_document->has_loaded_once());

  host_observer_.Observe(offscreen_document);

  // Add a reference so that we can respond to the extension once the
  // offscreen document finishes its initial load.
  // Balanced in either `OnBrowserContextShutdown()` or
  // `SendResponseToExtension()`.
  AddRef();

  return RespondLater();
}

void OffscreenCreateDocumentFunction::OnBrowserContextShutdown() {
  // Release dangling lifetime pointers and bail. No point in responding now;
  // the context is shutting down. Reset `host_observer_` first to allay any
  // re-entrancy concerns about the host being destructed at this point.
  host_observer_.Reset();
  Release();  // Balanced in Run().
}

void OffscreenCreateDocumentFunction::OnExtensionHostDestroyed(
    ExtensionHost* host) {
  SendResponseToExtension(
      Error("Offscreen document closed before fully loading."));
  // The host is destroyed, so ensure we're no longer observing it.
  DCHECK(!host_observer_.IsObserving());
}

void OffscreenCreateDocumentFunction::OnExtensionHostDidStopFirstLoad(
    const ExtensionHost* host) {
  SendResponseToExtension(NoArguments());
}

void OffscreenCreateDocumentFunction::SendResponseToExtension(
    ResponseValue response_value) {
  DCHECK(browser_context())
      << "SendResponseToExtension() should never be called after context "
      << "shutdown";

  // Even though the function is destroyed after responding to the extension,
  // this process happens asynchronously. Stop observing the host now to avoid
  // any chance of being notified of future events.
  host_observer_.Reset();

  Respond(std::move(response_value));
  Release();  // Balanced in Run().
}

OffscreenCloseDocumentFunction::OffscreenCloseDocumentFunction() = default;
OffscreenCloseDocumentFunction::~OffscreenCloseDocumentFunction() = default;

ExtensionFunction::ResponseAction OffscreenCloseDocumentFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(extension());

  OffscreenDocumentManager* manager =
      GetManagerToUse(*browser_context(), *extension());
  OffscreenDocumentHost* offscreen_document =
      manager->GetOffscreenDocumentForExtension(*extension());
  if (!offscreen_document)
    return RespondNow(Error("No current offscreen document."));

  host_observer_.Observe(offscreen_document);

  // Add a reference so that we can respond to the extension once the
  // offscreen document finishes closing.
  // Balanced in either `OnBrowserContextShutdown()` or
  // `SendResponseToExtension()`.
  AddRef();
  manager->CloseOffscreenDocumentForExtension(*extension());

  return RespondLater();
}

void OffscreenCloseDocumentFunction::OnBrowserContextShutdown() {
  // Release dangling lifetime pointers and bail. No point in responding now;
  // the context is shutting down. Reset `host_observer_` first to allay any
  // re-entrancy concerns about the host being destructed at this point.
  host_observer_.Reset();
  Release();  // Balanced in Run().
}

void OffscreenCloseDocumentFunction::OnExtensionHostDestroyed(
    ExtensionHost* host) {
  SendResponseToExtension(NoArguments());
  // The host is destroyed, so ensure we're no longer observing it.
  DCHECK(!host_observer_.IsObserving());
}

void OffscreenCloseDocumentFunction::SendResponseToExtension(
    ResponseValue response_value) {
  DCHECK(browser_context())
      << "SendResponseToExtension() should never be called after context "
      << "shutdown";

  // Even though the function is destroyed after responding to the extension,
  // this process happens asynchronously. Stop observing the host now to avoid
  // any chance of being notified of future events.
  host_observer_.Reset();

  Respond(std::move(response_value));
  Release();  // Balanced in Run().
}

OffscreenHasDocumentFunction::OffscreenHasDocumentFunction() = default;
OffscreenHasDocumentFunction::~OffscreenHasDocumentFunction() = default;

ExtensionFunction::ResponseAction OffscreenHasDocumentFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(extension());

  bool has_document =
      GetManagerToUse(*browser_context(), *extension())
          ->GetOffscreenDocumentForExtension(*extension()) != nullptr;
  return RespondNow(OneArgument(base::Value(has_document)));
}

}  // namespace extensions
