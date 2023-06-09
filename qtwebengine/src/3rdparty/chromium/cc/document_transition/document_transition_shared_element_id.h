// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_DOCUMENT_TRANSITION_DOCUMENT_TRANSITION_SHARED_ELEMENT_ID_H_
#define CC_DOCUMENT_TRANSITION_DOCUMENT_TRANSITION_SHARED_ELEMENT_ID_H_

#include <stdint.h>

#include <string>
#include <tuple>

#include "base/containers/flat_set.h"
#include "cc/cc_export.h"

namespace cc {

class CC_EXPORT DocumentTransitionSharedElementId {
 public:
  DocumentTransitionSharedElementId();
  explicit DocumentTransitionSharedElementId(uint32_t document_tag);
  DocumentTransitionSharedElementId(const DocumentTransitionSharedElementId&);
  DocumentTransitionSharedElementId(DocumentTransitionSharedElementId&&);
  ~DocumentTransitionSharedElementId();

  // Add a shared index to this id. It must have a valid document tag.
  void AddIndex(uint32_t index);

  // Returns true if the document tag matches this id and the index is in the
  // list of indices for this id.
  bool Matches(uint32_t document_tag, uint32_t index) const;

  DocumentTransitionSharedElementId& operator=(
      DocumentTransitionSharedElementId&&) = default;

  DocumentTransitionSharedElementId& operator=(
      const DocumentTransitionSharedElementId&) = default;

  bool operator==(const DocumentTransitionSharedElementId& other) const {
    return element_indices_ == other.element_indices_ &&
           document_tag_ == other.document_tag_;
  }

  bool operator!=(const DocumentTransitionSharedElementId& other) const {
    return !(*this == other);
  }

  bool operator<(const DocumentTransitionSharedElementId& other) const {
    return std::tie(document_tag_, element_indices_) <
           std::tie(other.document_tag_, other.element_indices_);
  }

  bool valid() const {
    return document_tag_ != 0u && !element_indices_.empty();
  }

  std::string ToString() const;

 private:
  uint32_t document_tag_ = 0u;
  base::flat_set<uint32_t> element_indices_;
};

}  // namespace cc

#endif  // CC_DOCUMENT_TRANSITION_DOCUMENT_TRANSITION_SHARED_ELEMENT_ID_H_
