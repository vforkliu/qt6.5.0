// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_TEST_AX_TREE_MANAGER_H_
#define UI_ACCESSIBILITY_TEST_AX_TREE_MANAGER_H_

#include <memory>

#include "ui/accessibility/ax_node_position.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/ax_tree_id.h"
#include "ui/accessibility/ax_tree_manager.h"

namespace ui {

class AXNode;
struct AXTreeUpdate;
struct TestAXTreeUpdateNode;

// A basic implementation of AXTreeManager that can be used in tests.
//
// For simplicity, this class supports only a single tree and doesn't perform
// any walking across multiple trees.
class TestAXTreeManager : public AXTreeManager {
 public:
  // This constructor does not create an empty AXTree. Call "SetTree" if you
  // need to manage a specific tree. Useful when you need to test for the
  // situation when no AXTree has been loaded yet.
  TestAXTreeManager();

  // Takes ownership of |tree|.
  explicit TestAXTreeManager(std::unique_ptr<AXTree> tree);

  ~TestAXTreeManager() override;

  TestAXTreeManager(const TestAXTreeManager& manager) = delete;
  TestAXTreeManager& operator=(const TestAXTreeManager& manager) = delete;

  TestAXTreeManager(TestAXTreeManager&& manager);
  TestAXTreeManager& operator=(TestAXTreeManager&& manager);

  void DestroyTree();
  AXTree* GetTree() const;

  // Takes ownership of |tree|.
  void SetTree(std::unique_ptr<AXTree> tree);

  // Creates and set the tree by a given AXTreeUpdate instance.
  AXTree* Init(AXTreeUpdate tree_update);

  // Set the tree by a given TestAXTreeUpdateNode instance.
  AXTree* Init(const TestAXTreeUpdateNode& tree_update_root);

  // Convenience functions to initialize directly from a few AXNodeData objects.
  AXTree* Init(const AXNodeData& node1,
               const AXNodeData& node2 = AXNodeData(),
               const AXNodeData& node3 = AXNodeData(),
               const AXNodeData& node4 = AXNodeData(),
               const AXNodeData& node5 = AXNodeData(),
               const AXNodeData& node6 = AXNodeData(),
               const AXNodeData& node7 = AXNodeData(),
               const AXNodeData& node8 = AXNodeData(),
               const AXNodeData& node9 = AXNodeData(),
               const AXNodeData& node10 = AXNodeData(),
               const AXNodeData& node11 = AXNodeData(),
               const AXNodeData& node12 = AXNodeData());

  // Create an AXPosition instance, a simple wrapper around
  // AXNodePosition::CreateTextPosition.
  AXNodePosition::AXPositionInstance CreateTextPosition(
      const AXNode& anchor,
      int text_offset,
      ax::mojom::TextAffinity affinity) const;

  // Create AXPosition instance for the given |anchor_id| belonging to the
  // current tree.
  AXNodePosition::AXPositionInstance CreateTextPosition(
      const AXNodeID& anchor_id,
      int text_offset,
      ax::mojom::TextAffinity affinity) const;

  // AXTreeManager implementation.
  AXNode* GetNodeFromTree(const AXTreeID& tree_id,
                          const AXNodeID node_id) const override;
  AXNode* GetParentNodeFromParentTreeAsAXNode() const override;
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_TEST_AX_TREE_MANAGER_H_
