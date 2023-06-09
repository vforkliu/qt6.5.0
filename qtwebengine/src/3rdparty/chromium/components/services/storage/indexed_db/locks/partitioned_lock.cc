// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/storage/indexed_db/locks/partitioned_lock.h"

#include <ostream>

namespace content {

PartitionedLock::PartitionedLock() = default;

PartitionedLock::~PartitionedLock() {
  Release();
}

PartitionedLock::PartitionedLock(PartitionedLock&& other) noexcept {
  DCHECK(!this->is_locked())
      << "Cannot move a lock onto an active lock: " << *this;
  this->range_ = std::move(other.range_);
  this->level_ = other.level_;
  this->lock_released_callback_ = std::move(other.lock_released_callback_);
  DCHECK(!other.is_locked());
}
PartitionedLock::PartitionedLock(PartitionedLockRange range,
                                 int level,
                                 LockReleasedCallback lock_released_callback)
    : range_(std::move(range)),
      level_(level),
      lock_released_callback_(std::move(lock_released_callback)) {}

PartitionedLock& PartitionedLock::operator=(PartitionedLock&& other) noexcept {
  DCHECK(!this->is_locked())
      << "Cannot move a lock onto an active lock: " << *this;
  this->range_ = std::move(other.range_);
  this->level_ = other.level_;
  this->lock_released_callback_ = std::move(other.lock_released_callback_);
  DCHECK(!other.is_locked());
  return *this;
}

void PartitionedLock::Release() {
  if (is_locked())
    std::move(lock_released_callback_).Run(level_, range_);
}

std::ostream& operator<<(std::ostream& out, const PartitionedLock& lock) {
  return out << "<PartitionedLock>{is_locked_: " << lock.is_locked()
             << ", level_: " << lock.level() << ", range_: " << lock.range()
             << "}";
}

bool operator<(const PartitionedLock& x, const PartitionedLock& y) {
  if (x.level() != y.level())
    return x.level() < y.level();
  if (x.range().begin != y.range().begin)
    return x.range().begin < y.range().begin;
  return x.range().end < y.range().end;
}
bool operator==(const PartitionedLock& x, const PartitionedLock& y) {
  return x.level() == y.level() && x.range().begin == y.range().begin &&
         x.range().end == y.range().end;
}
bool operator!=(const PartitionedLock& x, const PartitionedLock& y) {
  return !(x == y);
}

}  // namespace content
