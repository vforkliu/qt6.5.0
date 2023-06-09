// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.browserfragment.interfaces;

import org.chromium.browserfragment.interfaces.IStringCallback;
import org.chromium.browserfragment.interfaces.IWebMessageCallback;

import java.util.List;
import org.chromium.browserfragment.interfaces.ITabObserverDelegate;

oneway interface ITabProxy {
  void setActive() = 1;
  void close() = 2;
  void executeScript(in String script, in boolean useSeparateIsolate, in IStringCallback callback) = 3;

  void registerWebMessageCallback(in IWebMessageCallback callback, in String jsObjectName, in List<String> allowedOrigins) = 4;
  void unregisterWebMessageCallback(in String jsObjectName) = 5;

  void setTabObserverDelegate(ITabObserverDelegate tabObserverDelegate) = 6;
}
