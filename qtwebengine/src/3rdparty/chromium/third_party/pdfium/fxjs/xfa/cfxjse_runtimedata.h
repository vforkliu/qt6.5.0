// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CFXJSE_RUNTIMEDATA_H_
#define FXJS_XFA_CFXJSE_RUNTIMEDATA_H_

#include <memory>

#include "fxjs/cfxjs_engine.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-persistent-handle.h"

class CFXJSE_RuntimeData final : public FXJS_PerIsolateData::ExtensionIface {
 public:
  CFXJSE_RuntimeData(const CFXJSE_RuntimeData&) = delete;
  CFXJSE_RuntimeData& operator=(const CFXJSE_RuntimeData&) = delete;
  ~CFXJSE_RuntimeData() override;

  static CFXJSE_RuntimeData* Get(v8::Isolate* pIsolate);

  const v8::Global<v8::Context>& GetRootContext() { return m_hRootContext; }

 private:
  static std::unique_ptr<CFXJSE_RuntimeData> Create(v8::Isolate* pIsolate);

  CFXJSE_RuntimeData();

  v8::Global<v8::FunctionTemplate> m_hRootContextGlobalTemplate;
  v8::Global<v8::Context> m_hRootContext;
};

#endif  // FXJS_XFA_CFXJSE_RUNTIMEDATA_H_
