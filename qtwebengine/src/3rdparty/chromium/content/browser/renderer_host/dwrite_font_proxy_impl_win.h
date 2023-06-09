// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_DWRITE_FONT_PROXY_IMPL_WIN_H_
#define CONTENT_BROWSER_RENDERER_HOST_DWRITE_FONT_PROXY_IMPL_WIN_H_

#include <dwrite.h>
#include <dwrite_2.h>
#include <dwrite_3.h>
#include <wrl.h>

#include <string>
#include <vector>

#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/dwrite_font_lookup_table_builder_win.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "third_party/blink/public/mojom/dwrite_font_proxy/dwrite_font_proxy.mojom.h"

namespace content {

// Implements a message filter that handles the dwrite font proxy messages.
// If DWrite is enabled, calls into the system font collection to obtain
// results. Otherwise, acts as if the system collection contains no fonts.
class CONTENT_EXPORT DWriteFontProxyImpl
    : public blink::mojom::DWriteFontProxy {
 public:
  DWriteFontProxyImpl();

  DWriteFontProxyImpl(const DWriteFontProxyImpl&) = delete;
  DWriteFontProxyImpl& operator=(const DWriteFontProxyImpl&) = delete;

  ~DWriteFontProxyImpl() override;

  static void Create(
      mojo::PendingReceiver<blink::mojom::DWriteFontProxy> receiver);

  void SetWindowsFontsPathForTesting(std::u16string path);

 protected:
  // blink::mojom::DWriteFontProxy:
  void FindFamily(const std::u16string& family_name,
                  FindFamilyCallback callback) override;
  void GetFamilyCount(GetFamilyCountCallback callback) override;
  void GetFamilyNames(uint32_t family_index,
                      GetFamilyNamesCallback callback) override;
  void GetFontFiles(uint32_t family_index,
                    GetFontFilesCallback callback) override;
  void MapCharacters(const std::u16string& text,
                     blink::mojom::DWriteFontStylePtr font_style,
                     const std::u16string& locale_name,
                     uint32_t reading_direction,
                     const std::u16string& base_family_name,
                     MapCharactersCallback callback) override;
  void MatchUniqueFont(const std::u16string& unique_font_name,
                       MatchUniqueFontCallback callback) override;
  void GetUniqueFontLookupMode(
      GetUniqueFontLookupModeCallback callback) override;

  void GetUniqueNameLookupTableIfAvailable(
      GetUniqueNameLookupTableIfAvailableCallback callback) override;

  void GetUniqueNameLookupTable(
      GetUniqueNameLookupTableCallback callback) override;

  void FallbackFamilyAndStyleForCodepoint(
      const std::string& base_family_name,
      const std::string& locale_name,
      uint32_t codepoint,
      FallbackFamilyAndStyleForCodepointCallback callback) override;

  void InitializeDirectWrite();

 private:
  bool IsLastResortFallbackFont(uint32_t font_index);

 private:
  bool direct_write_initialized_ = false;
  Microsoft::WRL::ComPtr<IDWriteFontCollection> collection_;
  Microsoft::WRL::ComPtr<IDWriteFactory> factory_;
  Microsoft::WRL::ComPtr<IDWriteFactory2> factory2_;
  Microsoft::WRL::ComPtr<IDWriteFactory3> factory3_;
  Microsoft::WRL::ComPtr<IDWriteFontFallback> font_fallback_;
  std::u16string windows_fonts_path_;
  base::MappedReadOnlyRegion font_unique_name_table_memory_;

  // Temp code to help track down crbug.com/561873
  std::vector<uint32_t> last_resort_fonts_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_DWRITE_FONT_PROXY_IMPL_WIN_H_
