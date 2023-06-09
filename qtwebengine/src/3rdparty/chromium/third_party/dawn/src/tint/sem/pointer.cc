// Copyright 2020 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/tint/sem/pointer.h"

#include "src/tint/program_builder.h"
#include "src/tint/sem/reference.h"
#include "src/tint/utils/hash.h"

TINT_INSTANTIATE_TYPEINFO(tint::sem::Pointer);

namespace tint::sem {

Pointer::Pointer(const Type* subtype, ast::AddressSpace address_space, ast::Access access)
    : subtype_(subtype), address_space_(address_space), access_(access) {
    TINT_ASSERT(Semantic, !subtype->Is<Reference>());
    TINT_ASSERT(Semantic, access != ast::Access::kUndefined);
}

size_t Pointer::Hash() const {
    return utils::Hash(TypeInfo::Of<Pointer>().full_hashcode, address_space_, subtype_, access_);
}

bool Pointer::Equals(const sem::Type& other) const {
    if (auto* o = other.As<Pointer>()) {
        return o->address_space_ == address_space_ && o->subtype_ == subtype_ &&
               o->access_ == access_;
    }
    return false;
}

std::string Pointer::FriendlyName(const SymbolTable& symbols) const {
    std::ostringstream out;
    out << "ptr<";
    if (address_space_ != ast::AddressSpace::kNone) {
        out << address_space_ << ", ";
    }
    out << subtype_->FriendlyName(symbols) << ", " << access_;
    out << ">";
    return out.str();
}

Pointer::Pointer(Pointer&&) = default;

Pointer::~Pointer() = default;

}  // namespace tint::sem
