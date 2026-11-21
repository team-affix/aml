#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_episode.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockRunEpisode {
    MOCK_METHOD(episode_outcome, run, (const bin_view&));
};

struct MockSetReward {
    MOCK_METHOD(void, set_value, (double));
};

struct MockTerminate {
    MOCK_METHOD(void, terminate, ());
};

struct MctsEpisodeTest : public ::testing::Test {
    MctsEpisodeTest();
    episode_outcome costing(size_t cost) const;
    value label;
    bin_view training;
    model_node model;
    NiceMock<MockRunEpisode> run_episode;
    NiceMock<MockSetReward> set_reward;
    NiceMock<MockTerminate> terminate;
    mcts_episode<NiceMock<MockRunEpisode>, NiceMock<MockSetReward>, NiceMock<MockTerminate>> episode{
        run_episode, set_reward, terminate};
};

MctsEpisodeTest::MctsEpisodeTest()
    : label{.ctor = 0, .args = {}}
    , training(bin_view::of(bin{.inputs = {}, .labels = typed_column{.type = 0, .cells = {label}}}))
    , model{model_node::leaf{.output = feature{feature::constant{.literal = label}}}}
    , run_episode()
    , set_reward()
    , terminate() {}

episode_outcome MctsEpisodeTest::costing(size_t cost) const {
    return episode_outcome{.model = model, .cost = cost};
}

}

TEST_F(MctsEpisodeTest, TheNegatedCostRewardsThePathBeforeTheEpisodeEnds) {
    EXPECT_CALL(run_episode, run(_)).Times(AnyNumber());
    EXPECT_CALL(set_reward, set_value(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(run_episode, run(training)).WillOnce(Return(costing(7)));
    EXPECT_CALL(set_reward, set_value(-7.0));
    EXPECT_CALL(terminate, terminate());
    episode.run(training);
}

TEST_F(MctsEpisodeTest, AFreeModelIsRewardedWithZero) {
    ON_CALL(run_episode, run(training)).WillByDefault(Return(costing(0)));
    EXPECT_CALL(set_reward, set_value(_)).Times(AnyNumber());
    EXPECT_CALL(set_reward, set_value(0.0));
    episode.run(training);
}

TEST_F(MctsEpisodeTest, ACostlierModelEarnsALowerReward) {
    EXPECT_CALL(run_episode, run(_)).Times(AnyNumber());
    EXPECT_CALL(set_reward, set_value(_)).Times(AnyNumber());
    EXPECT_CALL(run_episode, run(training)).WillOnce(Return(costing(3))).WillOnce(Return(costing(9)));
    InSequence order;
    EXPECT_CALL(set_reward, set_value(-3.0));
    EXPECT_CALL(set_reward, set_value(-9.0));
    episode.run(training);
    episode.run(training);
}

TEST_F(MctsEpisodeTest, EveryRunEndsItsMctsEpisode) {
    EXPECT_CALL(terminate, terminate()).Times(3);
    episode.run(training);
    episode.run(training);
    episode.run(training);
}

TEST_F(MctsEpisodeTest, TheEpisodesModelAndCostAreReturned) {
    ON_CALL(run_episode, run(training)).WillByDefault(Return(costing(7)));
    EXPECT_EQ(episode.run(training), costing(7));
}
