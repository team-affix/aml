#include <set>
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

TEST_F(TreeWalkerTest, TheRootIsNeverHandedOutAgain) {
    for (mcts_choice choice = 0; choice < 10; ++choice)
        EXPECT_NE(walker.walk(0, choice), 0u);
}

TEST_F(TreeWalkerTest, FreshNodesAreAllDistinct) {
    std::set<mcts_tree_node_id> seen;
    mcts_tree_node_id node = 0;
    for (mcts_choice choice = 0; choice < 20; ++choice) {
        node = walker.walk(node, choice % 3);
        EXPECT_TRUE(seen.insert(node).second);
    }
}

TEST_F(TreeWalkerTest, WalkingAPathAgainRetracesItsNodes) {
    const mcts_tree_node_id first = walker.walk(0, 2);
    const mcts_tree_node_id second = walker.walk(first, 5);
    const mcts_tree_node_id third = walker.walk(second, 0);
    EXPECT_EQ(walker.walk(walker.walk(walker.walk(0, 2), 5), 0), third);
}
