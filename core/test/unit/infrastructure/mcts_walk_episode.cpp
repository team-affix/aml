#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_walk_episode.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockRunWalk {
    MOCK_METHOD(table_outcome, run, ());
};

struct MockSetReward {
    MOCK_METHOD(void, set_value, (double));
};

struct MockTerminate {
    MOCK_METHOD(void, terminate, ());
};

struct MctsWalkEpisodeTest : public ::testing::Test {
    MctsWalkEpisodeTest();
    table_outcome costing(size_t cost) const;
    table_model model;
    NiceMock<MockRunWalk> run_walk;
    NiceMock<MockSetReward> set_reward;
    NiceMock<MockTerminate> terminate;
    mcts_walk_episode<NiceMock<MockRunWalk>, NiceMock<MockSetReward>, NiceMock<MockTerminate>> episode{
        run_walk, set_reward, terminate};
};

MctsWalkEpisodeTest::MctsWalkEpisodeTest()
    : model{.graph = model_graph{.nodes = {}, .types = {}, .outputs = {3}}, .columns = {}, .rows = {}}
    , run_walk()
    , set_reward()
    , terminate() {}

table_outcome MctsWalkEpisodeTest::costing(size_t cost) const {
    return table_outcome{.model = model, .cost = cost};
}

}

TEST_F(MctsWalkEpisodeTest, TheNegatedCostRewardsThePathBeforeTheEpisodeEnds) {
    EXPECT_CALL(set_reward, set_value(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(run_walk, run()).WillOnce(Return(costing(7)));
    EXPECT_CALL(set_reward, set_value(-7.0));
    EXPECT_CALL(terminate, terminate());
    episode.run();
}

TEST_F(MctsWalkEpisodeTest, TheWalksOutcomeIsReturned) {
    ON_CALL(run_walk, run()).WillByDefault(Return(costing(4)));
    const table_outcome outcome = episode.run();
    EXPECT_EQ(outcome.cost, 4u);
    EXPECT_EQ(outcome.model.graph, model.graph);
}
