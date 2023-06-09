// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_tilingpattern.h"

#include <math.h>

#include <utility>

#include "core/fpdfapi/page/cpdf_allstates.h"
#include "core/fpdfapi/page/cpdf_form.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "third_party/base/check.h"

CPDF_TilingPattern::CPDF_TilingPattern(CPDF_Document* pDoc,
                                       RetainPtr<CPDF_Object> pPatternObj,
                                       const CFX_Matrix& parentMatrix)
    : CPDF_Pattern(pDoc, std::move(pPatternObj), parentMatrix) {
  DCHECK(document());
  m_bColored = pattern_obj()->GetDict()->GetIntegerFor("PaintType") == 1;
  SetPatternToFormMatrix();
}

CPDF_TilingPattern::~CPDF_TilingPattern() = default;

CPDF_TilingPattern* CPDF_TilingPattern::AsTilingPattern() {
  return this;
}

std::unique_ptr<CPDF_Form> CPDF_TilingPattern::Load(CPDF_PageObject* pPageObj) {
  RetainPtr<const CPDF_Dictionary> pDict = pattern_obj()->GetDict();
  m_bColored = pDict->GetIntegerFor("PaintType") == 1;
  m_XStep = fabsf(pDict->GetFloatFor("XStep"));
  m_YStep = fabsf(pDict->GetFloatFor("YStep"));

  CPDF_Stream* pStream = pattern_obj()->AsMutableStream();
  if (!pStream)
    return nullptr;

  const CFX_Matrix& matrix = parent_matrix();
  auto form = std::make_unique<CPDF_Form>(document(), nullptr,
                                          pdfium::WrapRetain(pStream));

  CPDF_AllStates allStates;
  allStates.m_ColorState.Emplace();
  allStates.m_GraphState.Emplace();
  allStates.m_TextState.Emplace();
  allStates.m_GeneralState = pPageObj->m_GeneralState;
  form->ParseContent(&allStates, &matrix, nullptr);
  m_BBox = pDict->GetRectFor("BBox");
  return form;
}
