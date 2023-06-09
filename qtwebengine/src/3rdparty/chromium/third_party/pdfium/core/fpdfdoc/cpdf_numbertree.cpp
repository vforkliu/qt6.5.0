// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_numbertree.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"

namespace {

RetainPtr<const CPDF_Object> SearchNumberNode(const CPDF_Dictionary* pNode,
                                              int num) {
  RetainPtr<const CPDF_Array> pLimits = pNode->GetArrayFor("Limits");
  if (pLimits &&
      (num < pLimits->GetIntegerAt(0) || num > pLimits->GetIntegerAt(1))) {
    return nullptr;
  }
  RetainPtr<const CPDF_Array> pNumbers = pNode->GetArrayFor("Nums");
  if (pNumbers) {
    for (size_t i = 0; i < pNumbers->size() / 2; i++) {
      int index = pNumbers->GetIntegerAt(i * 2);
      if (num == index)
        return pNumbers->GetDirectObjectAt(i * 2 + 1);
      if (index > num)
        break;
    }
    return nullptr;
  }

  RetainPtr<const CPDF_Array> pKids = pNode->GetArrayFor("Kids");
  if (!pKids)
    return nullptr;

  for (size_t i = 0; i < pKids->size(); i++) {
    RetainPtr<const CPDF_Dictionary> pKid = pKids->GetDictAt(i);
    if (!pKid)
      continue;

    RetainPtr<const CPDF_Object> pFound = SearchNumberNode(pKid.Get(), num);
    if (pFound)
      return pFound;
  }
  return nullptr;
}

}  // namespace

CPDF_NumberTree::CPDF_NumberTree(RetainPtr<const CPDF_Dictionary> pRoot)
    : m_pRoot(std::move(pRoot)) {}

CPDF_NumberTree::~CPDF_NumberTree() = default;

RetainPtr<const CPDF_Object> CPDF_NumberTree::LookupValue(int num) const {
  return SearchNumberNode(m_pRoot.Get(), num);
}
