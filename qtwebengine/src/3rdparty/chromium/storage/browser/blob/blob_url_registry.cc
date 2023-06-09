// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_registry.h"

#include "base/check.h"
#include "storage/browser/blob/blob_url_utils.h"
#include "url/gurl.h"

namespace storage {

BlobUrlRegistry::BlobUrlRegistry(base::WeakPtr<BlobUrlRegistry> fallback)
    : fallback_(std::move(fallback)) {}

BlobUrlRegistry::~BlobUrlRegistry() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool BlobUrlRegistry::AddUrlMapping(
    const GURL& blob_url,
    mojo::PendingRemote<blink::mojom::Blob> blob,
    // TODO(https://crbug.com/1224926): Remove these once experiment is over.
    const base::UnguessableToken& unsafe_agent_cluster_id,
    const absl::optional<net::SchemefulSite>& unsafe_top_level_site) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!BlobUrlUtils::UrlHasFragment(blob_url));
  if (IsUrlMapped(blob_url))
    return false;
  url_to_unsafe_agent_cluster_id_[blob_url] = unsafe_agent_cluster_id;
  if (unsafe_top_level_site)
    url_to_unsafe_top_level_site_[blob_url] = *unsafe_top_level_site;
  url_to_blob_[blob_url] = std::move(blob);
  return true;
}

bool BlobUrlRegistry::RemoveUrlMapping(const GURL& blob_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!BlobUrlUtils::UrlHasFragment(blob_url));
  auto blob_it = url_to_blob_.find(blob_url);
  if (blob_it == url_to_blob_.end()) {
    return false;
  }
  url_to_blob_.erase(blob_it);
  url_to_unsafe_agent_cluster_id_.erase(blob_url);
  url_to_unsafe_top_level_site_.erase(blob_url);
  return true;
}

bool BlobUrlRegistry::IsUrlMapped(const GURL& blob_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (url_to_blob_.find(blob_url) != url_to_blob_.end())
    return true;
  if (fallback_)
    return fallback_->IsUrlMapped(blob_url);
  return false;
}

// TODO(https://crbug.com/1224926): Remove this once experiment is over.
absl::optional<base::UnguessableToken> BlobUrlRegistry::GetUnsafeAgentClusterID(
    const GURL& blob_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = url_to_unsafe_agent_cluster_id_.find(blob_url);
  if (it != url_to_unsafe_agent_cluster_id_.end())
    return it->second;
  if (fallback_)
    return fallback_->GetUnsafeAgentClusterID(blob_url);
  return absl::nullopt;
}

absl::optional<net::SchemefulSite> BlobUrlRegistry::GetUnsafeTopLevelSite(
    const GURL& blob_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = url_to_unsafe_top_level_site_.find(blob_url);
  if (it != url_to_unsafe_top_level_site_.end())
    return it->second;
  if (fallback_)
    return fallback_->GetUnsafeTopLevelSite(blob_url);
  return absl::nullopt;
}

mojo::PendingRemote<blink::mojom::Blob> BlobUrlRegistry::GetBlobFromUrl(
    const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = url_to_blob_.find(BlobUrlUtils::ClearUrlFragment(url));
  if (it == url_to_blob_.end())
    return fallback_ ? fallback_->GetBlobFromUrl(url) : mojo::NullRemote();
  mojo::Remote<blink::mojom::Blob> blob(std::move(it->second));
  mojo::PendingRemote<blink::mojom::Blob> result;
  blob->Clone(result.InitWithNewPipeAndPassReceiver());
  it->second = blob.Unbind();
  return result;
}

void BlobUrlRegistry::AddTokenMapping(
    const base::UnguessableToken& token,
    const GURL& url,
    mojo::PendingRemote<blink::mojom::Blob> blob) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(token_to_url_and_blob_.find(token) == token_to_url_and_blob_.end());
  token_to_url_and_blob_.emplace(token, std::make_pair(url, std::move(blob)));
}

void BlobUrlRegistry::RemoveTokenMapping(const base::UnguessableToken& token) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(token_to_url_and_blob_.find(token) != token_to_url_and_blob_.end());
  token_to_url_and_blob_.erase(token);
}

bool BlobUrlRegistry::GetTokenMapping(
    const base::UnguessableToken& token,
    GURL* url,
    mojo::PendingRemote<blink::mojom::Blob>* blob) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = token_to_url_and_blob_.find(token);
  if (it == token_to_url_and_blob_.end())
    return false;
  *url = it->second.first;
  mojo::Remote<blink::mojom::Blob> source_blob(std::move(it->second.second));
  source_blob->Clone(blob->InitWithNewPipeAndPassReceiver());
  it->second.second = source_blob.Unbind();
  return true;
}

}  // namespace storage
