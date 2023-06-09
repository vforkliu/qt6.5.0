// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_cache/public/features.h"
#include "base/feature_list.h"

namespace web_cache {

BASE_FEATURE(kTrimWebCacheOnMemoryPressureOnly,
             "TrimWebCacheOnMemoryPressureOnly",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace web_cache
