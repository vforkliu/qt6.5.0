// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_QUOTA_QUOTA_CALLBACKS_H_
#define STORAGE_BROWSER_QUOTA_QUOTA_CALLBACKS_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/contains.h"
#include "components/services/storage/public/cpp/buckets/bucket_locator.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom-forward.h"

namespace blink {
class StorageKey;
}

namespace storage {

struct UsageInfo;
using UsageInfoEntries = std::vector<UsageInfo>;

// Common callback types that are used throughout in the quota module.
using UsageCallback =
    base::OnceCallback<void(int64_t usage, int64_t unlimited_usage)>;
using QuotaCallback =
    base::OnceCallback<void(blink::mojom::QuotaStatusCode status,
                            int64_t quota)>;
using UsageWithBreakdownCallback =
    base::OnceCallback<void(int64_t usage,
                            blink::mojom::UsageBreakdownPtr usage_breakdown)>;
using StatusCallback = base::OnceCallback<void(blink::mojom::QuotaStatusCode)>;
using GetBucketsCallback =
    base::OnceCallback<void(const std::set<BucketLocator>& buckets,
                            blink::mojom::StorageType type)>;
using GetStorageKeysCallback =
    base::OnceCallback<void(const std::set<blink::StorageKey>& storage_keys)>;
using GetUsageInfoCallback = base::OnceCallback<void(UsageInfoEntries)>;
using GetBucketCallback =
    base::OnceCallback<void(const absl::optional<BucketLocator>& bucket_info)>;

// Simple template wrapper for a callback queue.
template <typename CallbackType, typename... Args>
class CallbackQueue {
 public:
  // Returns true if the given |callback| is the first one added to the queue.
  bool Add(CallbackType callback) {
    callbacks_.push_back(std::move(callback));
    return (callbacks_.size() == 1);
  }

  bool HasCallbacks() const { return !callbacks_.empty(); }

  // Runs the callbacks added to the queue and clears the queue.
  void Run(Args... args) {
    std::vector<CallbackType> callbacks;
    callbacks.swap(callbacks_);
    for (auto& callback : callbacks)
      std::move(callback).Run(args...);
  }

  void Swap(CallbackQueue<CallbackType, Args...>* other) {
    callbacks_.swap(other->callbacks_);
  }

  size_t size() const { return callbacks_.size(); }

 private:
  std::vector<CallbackType> callbacks_;
};

template <typename CallbackType, typename Key, typename... Args>
class CallbackQueueMap {
 public:
  using CallbackQueueType = CallbackQueue<CallbackType, Args...>;
  using CallbackMap = std::map<Key, CallbackQueueType>;
  using iterator = typename CallbackMap::iterator;

  bool Add(const Key& key, CallbackType callback) {
    return callback_map_[key].Add(std::move(callback));
  }

  bool HasCallbacks(const Key& key) const {
    return base::Contains(callback_map_, key);
  }

  bool HasAnyCallbacks() const { return !callback_map_.empty(); }

  iterator Begin() { return callback_map_.begin(); }
  iterator End() { return callback_map_.end(); }

  void Clear() { callback_map_.clear(); }

  // Runs the callbacks added for the given |key| and clears the key
  // from the map.
  template <typename... RunArgs>
  void Run(const Key& key, RunArgs&&... args) {
    if (!this->HasCallbacks(key))
      return;
    CallbackQueueType queue;
    queue.Swap(&callback_map_[key]);
    callback_map_.erase(key);
    queue.Run(std::forward<RunArgs>(args)...);
  }

 private:
  CallbackMap callback_map_;
};

}  // namespace storage

#endif  // STORAGE_BROWSER_QUOTA_QUOTA_CALLBACKS_H_
