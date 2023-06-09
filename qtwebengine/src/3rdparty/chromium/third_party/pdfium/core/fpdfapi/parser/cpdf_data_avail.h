// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PARSER_CPDF_DATA_AVAIL_H_
#define CORE_FPDFAPI_PARSER_CPDF_DATA_AVAIL_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_CrossRefAvail;
class CPDF_Dictionary;
class CPDF_HintTables;
class CPDF_IndirectObjectHolder;
class CPDF_LinearizedHeader;
class CPDF_PageObjectAvail;
class CPDF_ReadValidator;
class CPDF_SyntaxParser;

class CPDF_DataAvail final : public Observable::ObserverIface {
 public:
  // Must match PDF_DATA_* definitions in public/fpdf_dataavail.h, but cannot
  // #include that header. fpdfsdk/fpdf_dataavail.cpp has static_asserts
  // to make sure the two sets of values match.
  enum DocAvailStatus {
    kDataError = -1,        // PDF_DATA_ERROR
    kDataNotAvailable = 0,  // PDF_DATA_NOTAVAIL
    kDataAvailable = 1,     // PDF_DATA_AVAIL
  };

  // Must match PDF_*LINEAR* definitions in public/fpdf_dataavail.h, but cannot
  // #include that header. fpdfsdk/fpdf_dataavail.cpp has static_asserts
  // to make sure the two sets of values match.
  enum DocLinearizationStatus {
    kLinearizationUnknown = -1,  // PDF_LINEARIZATION_UNKNOWN
    kNotLinearized = 0,          // PDF_NOT_LINEARIZED
    kLinearized = 1,             // PDF_LINEARIZED
  };

  // Must match PDF_FORM_* definitions in public/fpdf_dataavail.h, but cannot
  // #include that header. fpdfsdk/fpdf_dataavail.cpp has static_asserts
  // to make sure the two sets of values match.
  enum DocFormStatus {
    kFormError = -1,        // PDF_FORM_ERROR
    kFormNotAvailable = 0,  // PDF_FORM_NOTAVAIL
    kFormAvailable = 1,     // PDF_FORM_AVAIL
    kFormNotExist = 2,      // PDF_FORM_NOTEXIST
  };

  class FileAvail {
   public:
    virtual ~FileAvail();
    virtual bool IsDataAvail(FX_FILESIZE offset, size_t size) = 0;
  };

  class DownloadHints {
   public:
    virtual ~DownloadHints();
    virtual void AddSegment(FX_FILESIZE offset, size_t size) = 0;
  };

  CPDF_DataAvail(FileAvail* pFileAvail,
                 const RetainPtr<IFX_SeekableReadStream>& pFileRead);
  ~CPDF_DataAvail() override;

  // Observable::ObserverIface:
  void OnObservableDestroyed() override;

  DocAvailStatus IsDocAvail(DownloadHints* pHints);
  DocAvailStatus IsPageAvail(uint32_t dwPage, DownloadHints* pHints);
  DocFormStatus IsFormAvail(DownloadHints* pHints);
  DocLinearizationStatus IsLinearizedPDF();
  int GetPageCount() const;
  RetainPtr<const CPDF_Dictionary> GetPageDictionary(int index) const;
  RetainPtr<CPDF_ReadValidator> GetValidator() const;

  std::pair<CPDF_Parser::Error, std::unique_ptr<CPDF_Document>> ParseDocument(
      std::unique_ptr<CPDF_Document::RenderDataIface> pRenderData,
      std::unique_ptr<CPDF_Document::PageDataIface> pPageData,
      const ByteString& password);

  const CPDF_HintTables* GetHintTablesForTest() const {
    return m_pHintTables.get();
  }

 private:
  enum class InternalStatus : uint8_t {
    kHeader = 0,
    kFirstPage,
    kHintTable,
    kLoadAllCrossRef,
    kRoot,
    kInfo,
    kPageTree,
    kPage,
    kPageLaterLoad,
    kResources,
    kDone,
    kError,
    kLoadAllFile,
  };

  class PageNode {
   public:
    enum class Type { kUnknown = 0, kPage, kPages, kArray };

    PageNode();
    ~PageNode();

    Type m_type = Type::kUnknown;
    uint32_t m_dwPageNo = 0;
    std::vector<std::unique_ptr<PageNode>> m_ChildNodes;
  };

  static constexpr int kMaxPageRecursionDepth = 1024;

  bool CheckDocStatus();
  bool CheckHeader();
  bool CheckFirstPage();
  bool CheckHintTables();
  bool CheckRoot();
  bool CheckInfo();
  bool CheckPages();
  bool CheckPage();
  DocAvailStatus CheckResources(RetainPtr<CPDF_Dictionary> page);
  DocFormStatus CheckAcroForm();
  bool CheckPageStatus();

  DocAvailStatus CheckHeaderAndLinearized();
  RetainPtr<CPDF_Object> ParseIndirectObjectAt(
      FX_FILESIZE pos,
      uint32_t objnum,
      CPDF_IndirectObjectHolder* pObjList) const;
  RetainPtr<CPDF_Object> GetObject(uint32_t objnum, bool* pExistInFile);
  bool GetPageKids(CPDF_Object* pPages);
  bool PreparePageItem();
  bool LoadPages();
  bool CheckAndLoadAllXref();
  bool LoadAllFile();
  DocAvailStatus CheckLinearizedData();

  bool CheckPage(uint32_t dwPage);
  bool LoadDocPages();
  bool LoadDocPage(uint32_t dwPage);
  bool CheckPageNode(const PageNode& pageNode,
                     int32_t iPage,
                     int32_t& iCount,
                     int level);
  bool CheckUnknownPageNode(uint32_t dwPageNo, PageNode* pPageNode);
  bool CheckArrayPageNode(uint32_t dwPageNo, PageNode* pPageNode);
  bool CheckPageCount();
  bool IsFirstCheck(uint32_t dwPage);
  void ResetFirstCheck(uint32_t dwPage);
  bool ValidatePage(uint32_t dwPage) const;
  CPDF_SyntaxParser* GetSyntaxParser() const;

  RetainPtr<CPDF_ReadValidator> m_pFileRead;
  CPDF_Parser m_parser;
  RetainPtr<CPDF_Dictionary> m_pRoot;
  std::unique_ptr<CPDF_LinearizedHeader> m_pLinearized;
  bool m_bDocAvail = false;
  InternalStatus m_internalStatus = InternalStatus::kHeader;
  std::unique_ptr<CPDF_CrossRefAvail> m_pCrossRefAvail;
  const FX_FILESIZE m_dwFileLen;
  UnownedPtr<CPDF_Document> m_pDocument;
  std::vector<uint32_t> m_PageObjList;
  std::set<uint32_t> m_SeenPageObjList;
  uint32_t m_PagesObjNum = 0;
  bool m_bLinearedDataOK = false;
  bool m_bMainXRefLoadTried = false;
  bool m_bMainXRefLoadedOK = false;
  bool m_bPagesTreeLoad = false;
  bool m_bPagesLoad = false;
  std::unique_ptr<CPDF_PageObjectAvail> m_pFormAvail;
  std::vector<RetainPtr<CPDF_Object>> m_PagesArray;
  bool m_bTotalLoadPageTree = false;
  bool m_bCurPageDictLoadOK = false;
  bool m_bHeaderAvail = false;
  PageNode m_PageNode;
  std::set<uint32_t> m_pageMapCheckState;
  std::set<uint32_t> m_pagesLoadState;
  std::unique_ptr<CPDF_HintTables> m_pHintTables;
  std::map<uint32_t, std::unique_ptr<CPDF_PageObjectAvail>> m_PagesObjAvail;
  std::map<RetainPtr<const CPDF_Object>,
           std::unique_ptr<CPDF_PageObjectAvail>,
           std::less<>>
      m_PagesResourcesAvail;
};

#endif  // CORE_FPDFAPI_PARSER_CPDF_DATA_AVAIL_H_
