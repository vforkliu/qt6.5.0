// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_HANDLE_IMPL_H_
#define CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_HANDLE_IMPL_H_

#include "content/browser/preloading/prerender/prerender_host_registry.h"
#include "content/public/browser/prerender_handle.h"

namespace content {

class PrerenderHandleImpl final : public PrerenderHandle {
 public:
  PrerenderHandleImpl(
      base::WeakPtr<PrerenderHostRegistry> prerender_host_registry,
      int frame_tree_node_id,
      const GURL& url);
  ~PrerenderHandleImpl() override;
  GURL GetInitialPrerenderingUrl() override;
  base::WeakPtr<PrerenderHandle> GetWeakPtr() override;

 private:
  base::WeakPtr<PrerenderHostRegistry> prerender_host_registry_;
  // `frame_tree_node_id_` is the root FrameTreeNode id of the prerendered
  // page.
  const int frame_tree_node_id_;

  const GURL prerendering_url_;

  base::WeakPtrFactory<PrerenderHandle> weak_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_PRELOADING_PRERENDER_PRERENDER_HANDLE_IMPL_H_
