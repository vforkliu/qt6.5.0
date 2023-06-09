// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.browserfragment.interfaces;

import org.chromium.browserfragment.interfaces.ITabParams;

oneway interface ITabCallback {
    void onResult(in ITabParams tabParams) = 1;
}
