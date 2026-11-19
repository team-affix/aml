#include <set>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "infrastructure/tree_walker.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockIsInRollout {
    MOCK_METHOD(bool, is_in_rollout, (), (const));
};

struct TreeWalkerTest : public ::testing::Test {
    TreeWalkerTest();
    NiceMock<MockIsInRollout> rollout;
    tree_walker<NiceMock<MockIsInRollout>> walker;
};

TreeWalkerTest::TreeWalkerTest()
    : rollout()
    , walker(rollout) {
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(false));
}

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

TEST_F(TreeWalkerTest, DuringARolloutWalkingStaysAtTheNode) {
    const mcts_tree_node_id child = walker.walk(0, 3);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(true));
    EXPECT_EQ(walker.walk(child, 1), child);
    EXPECT_EQ(walker.walk(child, 2), child);
    EXPECT_EQ(walker.walk(0, 7), 0u);
}

TEST_F(TreeWalkerTest, DuringARolloutAKnownEdgeIsNotFollowed) {
    walker.walk(0, 3);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(true));
    EXPECT_EQ(walker.walk(0, 3), 0u);
}

TEST_F(TreeWalkerTest, ARolloutRecordsNoNodes) {
    const mcts_tree_node_id child = walker.walk(0, 3);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(true));
    for (mcts_choice choice = 0; choice < 100; ++choice)
        walker.walk(child, choice);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(false));
    // The first edge walked after the rollout gets the id right after the last one handed out before it.
    EXPECT_EQ(walker.walk(child, 100), child + 1);
}

TEST_F(TreeWalkerTest, EdgesKnownBeforeARolloutRetraceAfterIt) {
    const mcts_tree_node_id first = walker.walk(0, 2);
    const mcts_tree_node_id second = walker.walk(first, 5);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(true));
    walker.walk(second, 1);
    ON_CALL(rollout, is_in_rollout()).WillByDefault(Return(false));
    EXPECT_EQ(walker.walk(walker.walk(0, 2), 5), second);
}
