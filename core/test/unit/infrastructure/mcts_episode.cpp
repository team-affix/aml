#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_episode.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockRunEpisode {
    MOCK_METHOD(size_t, run, (const bin&));
};

struct MockSetReward {
    MOCK_METHOD(void, set_value, (double));
};

struct MockTerminate {
    MOCK_METHOD(void, terminate, ());
};

struct MctsEpisodeTest : public ::testing::Test {
    MctsEpisodeTest();
    value label;
    bin training;
    NiceMock<MockRunEpisode> run_episode;
    NiceMock<MockSetReward> set_reward;
    NiceMock<MockTerminate> terminate;
    mcts_episode<NiceMock<MockRunEpisode>, NiceMock<MockSetReward>, NiceMock<MockTerminate>> episode{
        run_episode, set_reward, terminate};
};

MctsEpisodeTest::MctsEpisodeTest()
    : label{.ctor = 0, .fields = {}}
    , training{.scope = {}, .labels = {&label}}
    , run_episode()
    , set_reward()
    , terminate() {}

}

TEST_F(MctsEpisodeTest, TheNegatedCostRewardsThePathBeforeTheEpisodeEnds) {
    InSequence order;
    EXPECT_CALL(run_episode, run(training)).WillOnce(Return(7));
    EXPECT_CALL(set_reward, set_value(-7.0));
    EXPECT_CALL(terminate, terminate());
    EXPECT_EQ(episode.run(training), 7u);
}

TEST_F(MctsEpisodeTest, AFreeModelIsRewardedWithZero) {
    ON_CALL(run_episode, run(training)).WillByDefault(Return(0));
    EXPECT_CALL(set_reward, set_value(0.0));
    EXPECT_EQ(episode.run(training), 0u);
}

TEST_F(MctsEpisodeTest, ACostlierModelEarnsALowerReward) {
    EXPECT_CALL(run_episode, run(training)).WillOnce(Return(3)).WillOnce(Return(9));
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
