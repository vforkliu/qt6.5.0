// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @return Whether a test module was loaded. */
export function loadTestModule(): boolean {
  const params = new URLSearchParams(window.location.search);
  const module = params.get('module');
  if (!module) {
    return false;
  }

  const host = params.get('host') || 'webui-test';
  if (host !== 'test' && host !== 'webui-test') {
    return false;
  }

  const script = document.createElement('script');
  script.type = 'module';
  script.src = `chrome://${host}/${module}`;
  document.body.appendChild(script);
  return true;
}
