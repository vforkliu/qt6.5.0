// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_stream.h"

#include <stdint.h>

#include <sstream>
#include <utility>

#include "constants/stream_dict_common.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_encryptor.h"
#include "core/fpdfapi/parser/cpdf_flateencoder.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/span_util.h"
#include "third_party/base/containers/contains.h"
#include "third_party/base/numerics/safe_conversions.h"

namespace {

bool IsMetaDataStreamDictionary(const CPDF_Dictionary* dict) {
  // See ISO 32000-1:2008 spec, table 315.
  return ValidateDictType(dict, "Metadata") &&
         dict->GetNameFor("Subtype") == "XML";
}

}  // namespace

CPDF_Stream::CPDF_Stream() = default;

CPDF_Stream::CPDF_Stream(pdfium::span<const uint8_t> pData,
                         RetainPtr<CPDF_Dictionary> pDict)
    : m_pDict(std::move(pDict)) {
  SetData(pData);
}

CPDF_Stream::CPDF_Stream(DataVector<uint8_t> pData,
                         RetainPtr<CPDF_Dictionary> pDict)
    : m_pDict(std::move(pDict)) {
  // TODO(crbug.com/pdfium/1872): Avoid copying.
  SetData(pData);
}

CPDF_Stream::CPDF_Stream(std::unique_ptr<uint8_t, FxFreeDeleter> pData,
                         size_t size,
                         RetainPtr<CPDF_Dictionary> pDict)
    : m_pDict(std::move(pDict)) {
  TakeDataInternal(std::move(pData), size);
}

CPDF_Stream::~CPDF_Stream() {
  m_ObjNum = kInvalidObjNum;
  if (m_pDict && m_pDict->GetObjNum() == kInvalidObjNum)
    m_pDict.Leak();  // lowercase release, release ownership.
}

CPDF_Object::Type CPDF_Stream::GetType() const {
  return kStream;
}

RetainPtr<const CPDF_Dictionary> CPDF_Stream::GetDict() const {
  return m_pDict;
}

CPDF_Stream* CPDF_Stream::AsMutableStream() {
  return this;
}

void CPDF_Stream::InitStream(pdfium::span<const uint8_t> pData,
                             RetainPtr<CPDF_Dictionary> pDict) {
  m_pDict = std::move(pDict);
  SetData(pData);
}

void CPDF_Stream::InitStreamFromFile(RetainPtr<IFX_SeekableReadStream> pFile,
                                     RetainPtr<CPDF_Dictionary> pDict) {
  m_bMemoryBased = false;
  m_pDataBuf.reset();
  m_pFile = std::move(pFile);
  m_pDict = std::move(pDict);
  m_dwSize = pdfium::base::checked_cast<size_t>(m_pFile->GetSize());
  m_pDict->SetNewFor<CPDF_Number>("Length",
                                  pdfium::base::checked_cast<int>(m_dwSize));
}

RetainPtr<CPDF_Object> CPDF_Stream::Clone() const {
  return CloneObjectNonCyclic(false);
}

RetainPtr<CPDF_Object> CPDF_Stream::CloneNonCyclic(
    bool bDirect,
    std::set<const CPDF_Object*>* pVisited) const {
  pVisited->insert(this);
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pdfium::WrapRetain(this));
  pAcc->LoadAllDataRaw();

  uint32_t streamSize = pAcc->GetSize();
  RetainPtr<const CPDF_Dictionary> pDict = GetDict();
  RetainPtr<CPDF_Dictionary> pNewDict;
  if (pDict && !pdfium::Contains(*pVisited, pDict.Get())) {
    pNewDict = ToDictionary(static_cast<const CPDF_Object*>(pDict.Get())
                                ->CloneNonCyclic(bDirect, pVisited));
  }
  return pdfium::MakeRetain<CPDF_Stream>(pAcc->DetachData(), streamSize,
                                         std::move(pNewDict));
}

void CPDF_Stream::SetDataAndRemoveFilter(pdfium::span<const uint8_t> pData) {
  SetData(pData);
  m_pDict->RemoveFor("Filter");
  m_pDict->RemoveFor(pdfium::stream::kDecodeParms);
}

void CPDF_Stream::SetDataFromStringstreamAndRemoveFilter(
    fxcrt::ostringstream* stream) {
  if (stream->tellp() <= 0) {
    SetDataAndRemoveFilter({});
    return;
  }

  SetDataAndRemoveFilter(
      {reinterpret_cast<const uint8_t*>(stream->str().c_str()),
       static_cast<size_t>(stream->tellp())});
}

void CPDF_Stream::SetData(pdfium::span<const uint8_t> pData) {
  std::unique_ptr<uint8_t, FxFreeDeleter> data_copy;
  if (!pData.empty()) {
    data_copy.reset(FX_AllocUninit(uint8_t, pData.size()));
    auto copy_span = pdfium::make_span(data_copy.get(), pData.size());
    fxcrt::spancpy(copy_span, pData);
  }
  TakeDataInternal(std::move(data_copy), pData.size());
}

void CPDF_Stream::TakeData(DataVector<uint8_t> data) {
  // TODO(crbug.com/pdfium/1872): Avoid copying.
  SetData(data);
}

void CPDF_Stream::TakeDataInternal(
    std::unique_ptr<uint8_t, FxFreeDeleter> pData,
    size_t size) {
  m_bMemoryBased = true;
  m_pFile = nullptr;
  m_pDataBuf = std::move(pData);
  m_dwSize = size;
  if (!m_pDict)
    m_pDict = pdfium::MakeRetain<CPDF_Dictionary>();
  m_pDict->SetNewFor<CPDF_Number>("Length",
                                  pdfium::base::checked_cast<int>(size));
}

void CPDF_Stream::SetDataFromStringstream(fxcrt::ostringstream* stream) {
  if (stream->tellp() <= 0) {
    SetData({});
    return;
  }
  SetData({reinterpret_cast<const uint8_t*>(stream->str().c_str()),
           static_cast<size_t>(stream->tellp())});
}

bool CPDF_Stream::ReadRawData(FX_FILESIZE offset,
                              uint8_t* buf,
                              size_t size) const {
  CHECK(!m_bMemoryBased);
  return m_pFile->ReadBlockAtOffset(buf, offset, size);
}

bool CPDF_Stream::HasFilter() const {
  return m_pDict && m_pDict->KeyExist("Filter");
}

WideString CPDF_Stream::GetUnicodeText() const {
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pdfium::WrapRetain(this));
  pAcc->LoadAllDataFiltered();
  return PDF_DecodeText(pAcc->GetSpan());
}

bool CPDF_Stream::WriteTo(IFX_ArchiveStream* archive,
                          const CPDF_Encryptor* encryptor) const {
  const bool is_metadata = IsMetaDataStreamDictionary(GetDict().Get());
  CPDF_FlateEncoder encoder(pdfium::WrapRetain(this), !is_metadata);

  DataVector<uint8_t> encrypted_data;
  pdfium::span<const uint8_t> data = encoder.GetSpan();
  if (encryptor && !is_metadata) {
    encrypted_data = encryptor->Encrypt(data);
    data = encrypted_data;
  }

  encoder.UpdateLength(data.size());
  if (!encoder.WriteDictTo(archive, encryptor))
    return false;

  if (!archive->WriteString("stream\r\n"))
    return false;

  if (!archive->WriteSpan(data))
    return false;

  return archive->WriteString("\r\nendstream");
}

const uint8_t* CPDF_Stream::GetInMemoryRawData() const {
  DCHECK(IsMemoryBased());
  return m_pDataBuf.get();
}
