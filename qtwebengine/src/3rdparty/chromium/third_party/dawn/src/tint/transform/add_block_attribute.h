// Copyright 2021 The Tint Authors.
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

#ifndef SRC_TINT_TRANSFORM_ADD_BLOCK_ATTRIBUTE_H_
#define SRC_TINT_TRANSFORM_ADD_BLOCK_ATTRIBUTE_H_

#include <string>

#include "src/tint/ast/internal_attribute.h"
#include "src/tint/transform/transform.h"

namespace tint::transform {

/// AddBlockAttribute is a transform that adds an
/// `@internal(block)` attribute to any structure that is used as the
/// store type of a buffer. If that structure is nested inside another structure
/// or an array, then it is wrapped inside another structure which gets the
/// `@internal(block)` attribute instead.
class AddBlockAttribute final : public Castable<AddBlockAttribute, Transform> {
  public:
    /// BlockAttribute is an InternalAttribute that is used to decorate a
    // structure that is used as a buffer in SPIR-V or GLSL.
    class BlockAttribute final : public Castable<BlockAttribute, ast::InternalAttribute> {
      public:
        /// Constructor
        /// @param program_id the identifier of the program that owns this node
        /// @param nid the unique node identifier
        BlockAttribute(ProgramID program_id, ast::NodeID nid);
        /// Destructor
        ~BlockAttribute() override;

        /// @return a short description of the internal attribute which will be
        /// displayed as `@internal(<name>)`
        std::string InternalName() const override;

        /// Performs a deep clone of this object using the CloneContext `ctx`.
        /// @param ctx the clone context
        /// @return the newly cloned object
        const BlockAttribute* Clone(CloneContext* ctx) const override;
    };

    /// Constructor
    AddBlockAttribute();

    /// Destructor
    ~AddBlockAttribute() override;

  protected:
    /// Runs the transform using the CloneContext built for transforming a
    /// program. Run() is responsible for calling Clone() on the CloneContext.
    /// @param ctx the CloneContext primed with the input program and
    /// ProgramBuilder
    /// @param inputs optional extra transform-specific input data
    /// @param outputs optional extra transform-specific output data
    void Run(CloneContext& ctx, const DataMap& inputs, DataMap& outputs) const override;
};

}  // namespace tint::transform

#endif  // SRC_TINT_TRANSFORM_ADD_BLOCK_ATTRIBUTE_H_
