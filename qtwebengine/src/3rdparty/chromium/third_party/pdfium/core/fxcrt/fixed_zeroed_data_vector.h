// Copyright 2022 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_FIXED_ZEROED_DATA_VECTOR_H_
#define CORE_FXCRT_FIXED_ZEROED_DATA_VECTOR_H_

#include "core/fxcrt/fixed_size_data_vector.h"

template <typename T>
using FixedZeroedDataVector =
    fxcrt::FixedSizeDataVector<T, /*initialize=*/true>;

#endif  // CORE_FXCRT_FIXED_ZEROED_DATA_VECTOR_H_
