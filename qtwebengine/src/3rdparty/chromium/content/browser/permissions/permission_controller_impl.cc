// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_
#define CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_

#include "content/browser/permissions/permission_controller_impl.h"
#include "base/bind.h"
#include "content/browser/permissions/permission_util.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/disallow_activation_reason.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom.h"
#include "url/origin.h"

namespace content {

namespace {

absl::optional<blink::scheduler::WebSchedulerTrackedFeature>
PermissionToSchedulingFeature(PermissionType permission_name) {
  switch (permission_name) {
    case PermissionType::NOTIFICATIONS:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedNotificationsPermission;
    case PermissionType::MIDI:
    case PermissionType::MIDI_SYSEX:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedMIDIPermission;
    case PermissionType::AUDIO_CAPTURE:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedAudioCapturePermission;
    case PermissionType::VIDEO_CAPTURE:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedVideoCapturePermission;
    case PermissionType::BACKGROUND_SYNC:
    case PermissionType::BACKGROUND_FETCH:
    case PermissionType::PERIODIC_BACKGROUND_SYNC:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedBackgroundWorkPermission;
    case PermissionType::STORAGE_ACCESS_GRANT:
      return blink::scheduler::WebSchedulerTrackedFeature::
          kRequestedStorageAccessGrant;
    case PermissionType::PROTECTED_MEDIA_IDENTIFIER:
    case PermissionType::DURABLE_STORAGE:
    case PermissionType::ACCESSIBILITY_EVENTS:
    case PermissionType::CLIPBOARD_READ_WRITE:
    case PermissionType::CLIPBOARD_SANITIZED_WRITE:
    case PermissionType::PAYMENT_HANDLER:
    case PermissionType::IDLE_DETECTION:
    case PermissionType::WAKE_LOCK_SCREEN:
    case PermissionType::WAKE_LOCK_SYSTEM:
    case PermissionType::NFC:
    case PermissionType::NUM:
    case PermissionType::SENSORS:
    case PermissionType::AR:
    case PermissionType::VR:
    case PermissionType::CAMERA_PAN_TILT_ZOOM:
    case PermissionType::WINDOW_PLACEMENT:
    case PermissionType::LOCAL_FONTS:
    case PermissionType::DISPLAY_CAPTURE:
    case PermissionType::GEOLOCATION:
      return absl::nullopt;
  }
}

const char kPermissionBlockedPortalsMessage[] =
    "%s permission has been blocked because it was requested inside a "
    "portal. "
    "Portals don't currently support permission requests.";

const char kPermissionBlockedFencedFrameMessage[] =
    "%s permission has been blocked because it was requested inside a fenced "
    "frame. Fenced frames don't currently support permission requests.";

void LogPermissionBlockedMessage(PermissionType permission,
                                 content::RenderFrameHost* rfh,
                                 const char* message) {
  rfh->GetOutermostMainFrame()->AddMessageToConsole(
      blink::mojom::ConsoleMessageLevel::kWarning,
      base::StringPrintf(message,
                         blink::GetPermissionString(permission).c_str()));
}

content::PermissionResult VerifyContextOfCurrentDocument(
    PermissionType permission,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);

  DCHECK(web_contents);

  // Permissions are denied for portals.
  if (web_contents->IsPortal()) {
    return PermissionResult(blink::mojom::PermissionStatus::DENIED,
                            PermissionStatusSource::PORTAL);
  }

  // Permissions are denied for fenced frames.
  if (render_frame_host->IsNestedWithinFencedFrame()) {
    return PermissionResult(blink::mojom::PermissionStatus::DENIED,
                            PermissionStatusSource::FENCED_FRAME);
  }

  return PermissionResult(blink::mojom::PermissionStatus::ASK,
                          PermissionStatusSource::UNSPECIFIED);
}

bool IsRequestAllowed(
    const std::vector<blink::PermissionType>& permissions,
    RenderFrameHost* render_frame_host,
    base::OnceCallback<
        void(const std::vector<blink::mojom::PermissionStatus>&)>& callback) {
  if (!render_frame_host) {
    // Permission request is not allowed without a valid RenderFrameHost.
    std::move(callback).Run(std::vector<blink::mojom::PermissionStatus>(
        permissions.size(), blink::mojom::PermissionStatus::ASK));
    return false;
  }

  // Verifies and evicts `render_frame_host` from BFcache. Returns true if
  // render_frame_host was evicted, returns false otherwise.
  if (render_frame_host->IsInactiveAndDisallowActivation(
          content::DisallowActivationReasonId::kRequestPermission)) {
    std::move(callback).Run(std::vector<blink::mojom::PermissionStatus>(
        permissions.size(), blink::mojom::PermissionStatus::ASK));
    return false;
  }

  // Verify each permission independently to generate proper warning messages.
  bool is_permission_allowed = true;
  for (PermissionType permission : permissions) {
    PermissionResult result =
        VerifyContextOfCurrentDocument(permission, render_frame_host);

    if (result.status == blink::mojom::PermissionStatus::DENIED) {
      switch (result.source) {
        case PermissionStatusSource::PORTAL:
          LogPermissionBlockedMessage(permission, render_frame_host,
                                      kPermissionBlockedPortalsMessage);
          break;
        case PermissionStatusSource::FENCED_FRAME:
          LogPermissionBlockedMessage(permission, render_frame_host,
                                      kPermissionBlockedFencedFrameMessage);
          break;
        default:
          break;
      }

      is_permission_allowed = false;
    }
  }

  if (!is_permission_allowed) {
    std::move(callback).Run(std::vector<blink::mojom::PermissionStatus>(
        permissions.size(), blink::mojom::PermissionStatus::DENIED));
    return false;
  }

  return true;
}

void NotifySchedulerAboutPermissionRequest(RenderFrameHost* render_frame_host,
                                           PermissionType permission_name) {
  DCHECK(render_frame_host);

  absl::optional<blink::scheduler::WebSchedulerTrackedFeature> feature =
      PermissionToSchedulingFeature(permission_name);

  if (!feature)
    return;

  static_cast<RenderFrameHostImpl*>(render_frame_host)
      ->OnBackForwardCacheDisablingStickyFeatureUsed(feature.value());
}

// Calls |original_cb|, a callback expecting the PermissionStatus of a set of
// permissions, after joining the results of overridden permissions and
// non-overridden permissions.
// |overridden_results| is an array of permissions that have already been
// overridden by DevTools.
// |delegated_results| contains results that did not have overrides - they
// were delegated - their results need to be inserted in order.
void MergeOverriddenAndDelegatedResults(
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        original_cb,
    std::vector<absl::optional<blink::mojom::PermissionStatus>>
        overridden_results,
    const std::vector<blink::mojom::PermissionStatus>& delegated_results) {
  std::vector<blink::mojom::PermissionStatus> full_results;
  full_results.reserve(overridden_results.size());
  auto delegated_it = delegated_results.begin();
  for (auto& status : overridden_results) {
    if (!status) {
      CHECK(delegated_it != delegated_results.end());
      status.emplace(*delegated_it++);
    }
    full_results.emplace_back(*status);
  }
  CHECK(delegated_it == delegated_results.end());

  std::move(original_cb).Run(full_results);
}

void PermissionStatusCallbackWrapper(
    base::OnceCallback<void(blink::mojom::PermissionStatus)> callback,
    const std::vector<blink::mojom::PermissionStatus>& vector) {
  DCHECK_EQ(1ul, vector.size());
  std::move(callback).Run(vector.at(0));
}

}  // namespace

PermissionControllerImpl::PermissionControllerImpl(
    BrowserContext* browser_context)
    : browser_context_(browser_context) {}

// TODO(https://crbug.com/1271543): Remove this method and use
// `PermissionController` instead. static
PermissionControllerImpl* PermissionControllerImpl::FromBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<PermissionControllerImpl*>(
      browser_context->GetPermissionController());
}

struct PermissionControllerImpl::Subscription {
  PermissionType permission;
  GURL requesting_origin;
  GURL embedding_origin;
  int render_frame_id = -1;
  int render_process_id = -1;
  base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback;
  // This is default-initialized to an invalid ID.
  PermissionControllerDelegate::SubscriptionId delegate_subscription_id;
};

PermissionControllerImpl::~PermissionControllerImpl() {
  // Ideally we need to unsubscribe from delegate subscriptions here,
  // but browser_context_ is already destroyed by this point, so
  // we can't fetch our delegate.
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetSubscriptionCurrentValue(
    const Subscription& subscription) {
  // The RFH may be null if the request is for a worker.
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      subscription.render_process_id, subscription.render_frame_id);
  if (rfh) {
    return GetPermissionStatusForCurrentDocument(subscription.permission, rfh);
  }

  content::RenderProcessHost* rph =
      content::RenderProcessHost::FromID(subscription.render_process_id);
  if (rph) {
    return GetPermissionStatusForWorker(
        subscription.permission, rph,
        url::Origin::Create(subscription.requesting_origin));
  }

  return GetPermissionStatusInternal(subscription.permission,
                                     subscription.requesting_origin,
                                     subscription.embedding_origin);
}

PermissionControllerImpl::SubscriptionsStatusMap
PermissionControllerImpl::GetSubscriptionsStatuses(
    const absl::optional<GURL>& origin) {
  SubscriptionsStatusMap statuses;
  for (SubscriptionsMap::iterator iter(&subscriptions_); !iter.IsAtEnd();
       iter.Advance()) {
    Subscription* subscription = iter.GetCurrentValue();
    if (origin.has_value() && subscription->requesting_origin != *origin)
      continue;
    statuses[iter.GetCurrentKey()] = GetSubscriptionCurrentValue(*subscription);
  }
  return statuses;
}

void PermissionControllerImpl::NotifyChangedSubscriptions(
    const SubscriptionsStatusMap& old_statuses) {
  std::vector<base::OnceClosure> callbacks;
  for (const auto& it : old_statuses) {
    auto key = it.first;
    Subscription* subscription = subscriptions_.Lookup(key);
    if (!subscription)
      continue;
    blink::mojom::PermissionStatus old_status = it.second;
    blink::mojom::PermissionStatus new_status =
        GetSubscriptionCurrentValue(*subscription);
    if (new_status != old_status)
      callbacks.push_back(base::BindOnce(subscription->callback, new_status));
  }
  for (auto& callback : callbacks)
    std::move(callback).Run();
}

PermissionControllerImpl::OverrideStatus
PermissionControllerImpl::GrantOverridesForDevTools(
    const absl::optional<url::Origin>& origin,
    const std::vector<PermissionType>& permissions) {
  return GrantPermissionOverrides(origin, permissions);
}

PermissionControllerImpl::OverrideStatus
PermissionControllerImpl::SetOverrideForDevTools(
    const absl::optional<url::Origin>& origin,
    PermissionType permission,
    const blink::mojom::PermissionStatus& status) {
  return SetPermissionOverride(origin, permission, status);
}

void PermissionControllerImpl::ResetOverridesForDevTools() {
  ResetPermissionOverrides();
}

PermissionControllerImpl::OverrideStatus
PermissionControllerImpl::SetPermissionOverride(
    const absl::optional<url::Origin>& origin,
    PermissionType permission,
    const blink::mojom::PermissionStatus& status) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (delegate && !delegate->IsPermissionOverridable(permission, origin)) {
    return OverrideStatus::kOverrideNotSet;
  }
  const auto old_statuses = GetSubscriptionsStatuses(
      origin ? absl::make_optional(origin->GetURL()) : absl::nullopt);
  permission_overrides_.Set(origin, permission, status);
  NotifyChangedSubscriptions(old_statuses);

  return OverrideStatus::kOverrideSet;
}

PermissionControllerImpl::OverrideStatus
PermissionControllerImpl::GrantPermissionOverrides(
    const absl::optional<url::Origin>& origin,
    const std::vector<PermissionType>& permissions) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (delegate) {
    for (const auto permission : permissions) {
      if (!delegate->IsPermissionOverridable(permission, origin))
        return OverrideStatus::kOverrideNotSet;
    }
  }

  const auto old_statuses = GetSubscriptionsStatuses(
      origin ? absl::make_optional(origin->GetURL()) : absl::nullopt);
  permission_overrides_.GrantPermissions(origin, permissions);
  // If any statuses changed because they lose overrides or the new overrides
  // modify their previous state (overridden or not), subscribers must be
  // notified manually.
  NotifyChangedSubscriptions(old_statuses);

  return OverrideStatus::kOverrideSet;
}

void PermissionControllerImpl::ResetPermissionOverrides() {
  const auto old_statuses = GetSubscriptionsStatuses();
  permission_overrides_ = PermissionOverrides();

  // If any statuses changed because they lost their overrides, the subscribers
  // must be notified manually.
  NotifyChangedSubscriptions(old_statuses);
}

void PermissionControllerImpl::RequestPermissions(
    const std::vector<blink::PermissionType>& permissions,
    RenderFrameHost* render_frame_host,
    const url::Origin& requested_origin,
    bool user_gesture,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!IsRequestAllowed(permissions, render_frame_host, callback)) {
    return;
  }

  for (PermissionType permission : permissions)
    NotifySchedulerAboutPermissionRequest(render_frame_host, permission);

  std::vector<PermissionType> permissions_without_overrides;
  std::vector<absl::optional<blink::mojom::PermissionStatus>> results;
  url::Origin origin = render_frame_host->GetLastCommittedOrigin();
  for (const auto& permission : permissions) {
    absl::optional<blink::mojom::PermissionStatus> override_status =
        permission_overrides_.Get(origin, permission);
    if (!override_status)
      permissions_without_overrides.push_back(permission);
    results.push_back(override_status);
  }

  auto wrapper = base::BindOnce(&MergeOverriddenAndDelegatedResults,
                                std::move(callback), results);
  if (permissions_without_overrides.empty()) {
    std::move(wrapper).Run({});
    return;
  }

  // Use delegate to find statuses of other permissions that have been requested
  // but do not have overrides.
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate) {
    std::move(wrapper).Run(std::vector<blink::mojom::PermissionStatus>(
        permissions_without_overrides.size(),
        blink::mojom::PermissionStatus::DENIED));
    return;
  }

  delegate->RequestPermissions(permissions_without_overrides, render_frame_host,
                               requested_origin.GetURL(), user_gesture,
                               std::move(wrapper));
}

void PermissionControllerImpl::RequestPermissionFromCurrentDocument(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    bool user_gesture,
    base::OnceCallback<void(blink::mojom::PermissionStatus)> callback) {
  RequestPermissionsFromCurrentDocument(
      {permission}, render_frame_host, user_gesture,
      base::BindOnce(&PermissionStatusCallbackWrapper, std::move(callback)));
}

void PermissionControllerImpl::RequestPermissionsFromCurrentDocument(
    const std::vector<PermissionType>& permissions,
    RenderFrameHost* render_frame_host,
    bool user_gesture,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!IsRequestAllowed(permissions, render_frame_host, callback))
    return;

  for (PermissionType permission : permissions)
    NotifySchedulerAboutPermissionRequest(render_frame_host, permission);

  std::vector<PermissionType> permissions_without_overrides;
  std::vector<absl::optional<blink::mojom::PermissionStatus>> results;
  url::Origin origin = render_frame_host->GetLastCommittedOrigin();
  for (const auto& permission : permissions) {
    absl::optional<blink::mojom::PermissionStatus> override_status =
        permission_overrides_.Get(origin, permission);
    if (!override_status)
      permissions_without_overrides.push_back(permission);
    results.push_back(override_status);
  }

  auto wrapper = base::BindOnce(&MergeOverriddenAndDelegatedResults,
                                std::move(callback), results);
  if (permissions_without_overrides.empty()) {
    std::move(wrapper).Run({});
    return;
  }

  // Use delegate to find statuses of other permissions that have been requested
  // but do not have overrides.
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate) {
    std::move(wrapper).Run(std::vector<blink::mojom::PermissionStatus>(
        permissions_without_overrides.size(),
        blink::mojom::PermissionStatus::DENIED));
    return;
  }

  delegate->RequestPermissionsFromCurrentDocument(
      permissions_without_overrides, render_frame_host, user_gesture,
      std::move(wrapper));
}

void PermissionControllerImpl::ResetPermission(blink::PermissionType permission,
                                               const url::Origin& origin) {
  ResetPermission(permission, origin.GetURL(), origin.GetURL());
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetPermissionStatusInternal(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  absl::optional<blink::mojom::PermissionStatus> status =
      permission_overrides_.Get(url::Origin::Create(requesting_origin),
                                permission);
  if (status)
    return *status;

  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return blink::mojom::PermissionStatus::DENIED;

  return delegate->GetPermissionStatus(permission, requesting_origin,
                                       embedding_origin);
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetPermissionStatusForWorker(
    PermissionType permission,
    RenderProcessHost* render_process_host,
    const url::Origin& worker_origin) {
  absl::optional<blink::mojom::PermissionStatus> status =
      permission_overrides_.Get(worker_origin, permission);
  if (status.has_value())
    return *status;

  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return blink::mojom::PermissionStatus::DENIED;
  return delegate->GetPermissionStatusForWorker(permission, render_process_host,
                                                worker_origin.GetURL());
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetPermissionStatusForCurrentDocument(
    PermissionType permission,
    RenderFrameHost* render_frame_host) {
  absl::optional<blink::mojom::PermissionStatus> status =
      permission_overrides_.Get(render_frame_host->GetLastCommittedOrigin(),
                                permission);
  if (status)
    return *status;

  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return blink::mojom::PermissionStatus::DENIED;

  if (VerifyContextOfCurrentDocument(permission, render_frame_host).status ==
      blink::mojom::PermissionStatus::DENIED) {
    return blink::mojom::PermissionStatus::DENIED;
  }

  return delegate->GetPermissionStatusForCurrentDocument(permission,
                                                         render_frame_host);
}

PermissionResult
PermissionControllerImpl::GetPermissionResultForCurrentDocument(
    PermissionType permission,
    RenderFrameHost* render_frame_host) {
  absl::optional<blink::mojom::PermissionStatus> status =
      permission_overrides_.Get(render_frame_host->GetLastCommittedOrigin(),
                                permission);
  if (status)
    return PermissionResult(*status, PermissionStatusSource::UNSPECIFIED);

  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return PermissionResult(blink::mojom::PermissionStatus::DENIED,
                            PermissionStatusSource::UNSPECIFIED);

  PermissionResult result =
      VerifyContextOfCurrentDocument(permission, render_frame_host);
  if (result.status == blink::mojom::PermissionStatus::DENIED)
    return result;

  return delegate->GetPermissionResultForCurrentDocument(permission,
                                                         render_frame_host);
}

PermissionResult
PermissionControllerImpl::GetPermissionResultForOriginWithoutContext(
    PermissionType permission,
    const url::Origin& origin) {
  absl::optional<blink::mojom::PermissionStatus> status =
      permission_overrides_.Get(origin, permission);
  if (status)
    return PermissionResult(*status, PermissionStatusSource::UNSPECIFIED);

  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return PermissionResult(blink::mojom::PermissionStatus::DENIED,
                            PermissionStatusSource::UNSPECIFIED);

  return delegate->GetPermissionResultForOriginWithoutContext(permission,
                                                              origin);
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetPermissionStatusForOriginWithoutContext(
    PermissionType permission,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  return GetPermissionStatusInternal(permission, requesting_origin.GetURL(),
                                     embedding_origin.GetURL());
}

void PermissionControllerImpl::ResetPermission(PermissionType permission,
                                               const GURL& requesting_origin,
                                               const GURL& embedding_origin) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return;
  delegate->ResetPermission(permission, requesting_origin, embedding_origin);
}

void PermissionControllerImpl::OnDelegatePermissionStatusChange(
    SubscriptionId subscription_id,
    blink::mojom::PermissionStatus status) {
  Subscription* subscription = subscriptions_.Lookup(subscription_id);
  DCHECK(subscription);
  // TODO(crbug.com/1223407) Adding this block to prevent crashes while we
  // investigate the root cause of the crash. This block will be removed as the
  // CHECK() above should be enough.
  if (!subscription)
    return;
  absl::optional<blink::mojom::PermissionStatus> status_override =
      permission_overrides_.Get(
          url::Origin::Create(subscription->requesting_origin),
          subscription->permission);
  if (!status_override.has_value())
    subscription->callback.Run(status);
}

PermissionControllerImpl::SubscriptionId
PermissionControllerImpl::SubscribePermissionStatusChange(
    PermissionType permission,
    RenderProcessHost* render_process_host,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::RepeatingCallback<void(blink::mojom::PermissionStatus)>&
        callback) {
  DCHECK(!render_process_host || !render_frame_host);
  auto subscription = std::make_unique<Subscription>();
  subscription->permission = permission;
  subscription->callback = callback;
  subscription->requesting_origin = requesting_origin;

  // The RFH may be null if the request is for a worker.
  if (render_frame_host) {
    subscription->embedding_origin =
        PermissionUtil::GetLastCommittedOriginAsURL(
            render_frame_host->GetMainFrame());
    subscription->render_frame_id = render_frame_host->GetRoutingID();
    subscription->render_process_id = render_frame_host->GetProcess()->GetID();
  } else {
    subscription->embedding_origin = requesting_origin;
    subscription->render_frame_id = -1;
    subscription->render_process_id =
        render_process_host ? render_process_host->GetID() : -1;
  }

  auto id = subscription_id_generator_.GenerateNextId();
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (delegate) {
    subscription->delegate_subscription_id =
        delegate->SubscribePermissionStatusChange(
            permission, render_process_host, render_frame_host,
            requesting_origin,
            base::BindRepeating(
                &PermissionControllerImpl::OnDelegatePermissionStatusChange,
                base::Unretained(this), id));
  }
  subscriptions_.AddWithID(std::move(subscription), id);
  return id;
}

PermissionControllerImpl::SubscriptionId
PermissionControllerImpl::SubscribePermissionStatusChange(
    PermissionType permission,
    RenderProcessHost* render_process_host,
    const url::Origin& requesting_origin,
    const base::RepeatingCallback<void(blink::mojom::PermissionStatus)>&
        callback) {
  return SubscribePermissionStatusChange(permission, render_process_host,
                                         /*render_frame_host=*/nullptr,
                                         requesting_origin.GetURL(), callback);
}

void PermissionControllerImpl::UnsubscribePermissionStatusChange(
    SubscriptionId subscription_id) {
  Subscription* subscription = subscriptions_.Lookup(subscription_id);
  if (!subscription)
    return;
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (delegate) {
    delegate->UnsubscribePermissionStatusChange(
        subscription->delegate_subscription_id);
  }
  subscriptions_.Remove(subscription_id);
}

}  // namespace content

#endif  // CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_
