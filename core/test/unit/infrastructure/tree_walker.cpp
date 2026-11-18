#include <gtest/gtest.h>
#include "infrastructure/tree_walker.hpp"

namespace {

struct TreeWalkerTest : public ::testing::Test {
    tree_walker walker;
};

}

TEST_F(TreeWalkerTest, AFirstWalkGivesAFreshNode) {
    EXPECT_EQ(walker.walk(0, 3), 1u);
    EXPECT_EQ(walker.walk(0, 4), 2u);
}

TEST_F(TreeWalkerTest, WalkingTheSameEdgeAgainGivesTheSameNode) {
    const mcts_tree_node_id child = walker.walk(0, 3);
    walker.walk(child, 0);
    EXPECT_EQ(walker.walk(0, 3), child);
}

TEST_F(TreeWalkerTest, TheSameChoiceFromDifferentNodesGivesDifferentNodes) {
    const mcts_tree_node_id child = walker.walk(0, 1);
    EXPECT_NE(walker.walk(child, 1), child);
}
