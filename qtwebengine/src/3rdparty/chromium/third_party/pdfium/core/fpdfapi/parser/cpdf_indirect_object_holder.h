// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PARSER_CPDF_INDIRECT_OBJECT_HOLDER_H_
#define CORE_FPDFAPI_PARSER_CPDF_INDIRECT_OBJECT_HOLDER_H_

#include <stdint.h>

#include <map>
#include <type_traits>
#include <utility>

#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/string_pool_template.h"
#include "core/fxcrt/weak_ptr.h"

class CPDF_IndirectObjectHolder {
 public:
  using const_iterator =
      std::map<uint32_t, RetainPtr<CPDF_Object>>::const_iterator;

  CPDF_IndirectObjectHolder();
  virtual ~CPDF_IndirectObjectHolder();

  RetainPtr<CPDF_Object> GetOrParseIndirectObject(uint32_t objnum);
  RetainPtr<const CPDF_Object> GetIndirectObject(uint32_t objnum) const;
  RetainPtr<CPDF_Object> GetMutableIndirectObject(uint32_t objnum);
  void DeleteIndirectObject(uint32_t objnum);

  // Creates and adds a new object owned by the indirect object holder,
  // and returns a retained pointer to it.  We have a special case to
  // handle objects that can intern strings from our ByteStringPool.
  template <typename T, typename... Args>
  typename std::enable_if<!CanInternStrings<T>::value, RetainPtr<T>>::type
  NewIndirect(Args&&... args) {
    return pdfium::WrapRetain(static_cast<T*>(
        AddIndirectObject(pdfium::MakeRetain<T>(std::forward<Args>(args)...))));
  }
  template <typename T, typename... Args>
  typename std::enable_if<CanInternStrings<T>::value, RetainPtr<T>>::type
  NewIndirect(Args&&... args) {
    return pdfium::WrapRetain(
        static_cast<T*>(AddIndirectObject(pdfium::MakeRetain<T>(
            m_pByteStringPool, std::forward<Args>(args)...))));
  }

  // Creates and adds a new object not owned by the indirect object holder,
  // but which can intern strings from it.
  template <typename T, typename... Args>
  typename std::enable_if<CanInternStrings<T>::value, RetainPtr<T>>::type New(
      Args&&... args) {
    return pdfium::MakeRetain<T>(m_pByteStringPool,
                                 std::forward<Args>(args)...);
  }

  // Takes ownership of |pObj|, returns unowned pointer to it.
  CPDF_Object* AddIndirectObject(RetainPtr<CPDF_Object> pObj);

  // Always takes ownership of |pObj|, return true if higher generation number.
  bool ReplaceIndirectObjectIfHigherGeneration(uint32_t objnum,
                                               RetainPtr<CPDF_Object> pObj);

  uint32_t GetLastObjNum() const { return m_LastObjNum; }
  void SetLastObjNum(uint32_t objnum) { m_LastObjNum = objnum; }

  WeakPtr<ByteStringPool> GetByteStringPool() const {
    return m_pByteStringPool;
  }

  const_iterator begin() const { return m_IndirectObjs.begin(); }
  const_iterator end() const { return m_IndirectObjs.end(); }

 protected:
  virtual RetainPtr<CPDF_Object> ParseIndirectObject(uint32_t objnum);

 private:
  friend class CPDF_Reference;

  const CPDF_Object* GetIndirectObjectInternal(uint32_t objnum) const;
  CPDF_Object* GetOrParseIndirectObjectInternal(uint32_t objnum);

  uint32_t m_LastObjNum = 0;
  std::map<uint32_t, RetainPtr<CPDF_Object>> m_IndirectObjs;
  WeakPtr<ByteStringPool> m_pByteStringPool;
};

#endif  // CORE_FPDFAPI_PARSER_CPDF_INDIRECT_OBJECT_HOLDER_H_
