#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rollout_switching_option_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockIsInRollout {
    MOCK_METHOD(bool, is_in_rollout, (), (const));
};

struct MockChooseInTree {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct MockChooseInRollout {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct RolloutSwitchingOptionChooserTest : public ::testing::Test {
    NiceMock<MockIsInRollout> is_in_rollout;
    NiceMock<MockChooseInTree> choose_in_tree;
    NiceMock<MockChooseInRollout> choose_in_rollout;
    rollout_switching_option_chooser<
        NiceMock<MockIsInRollout>, NiceMock<MockChooseInTree>, NiceMock<MockChooseInRollout>>
        chooser{is_in_rollout, choose_in_tree, choose_in_rollout};
};

}

TEST_F(RolloutSwitchingOptionChooserTest, InTheTreeTheTreesChooserChoosesFromTheKeys) {
    ON_CALL(is_in_rollout, is_in_rollout()).WillByDefault(Return(false));
    EXPECT_CALL(choose_in_rollout, choose(_)).Times(0);
    EXPECT_CALL(choose_in_tree, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tree, choose(std::vector<size_t>{0, 4, 2})).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose({0, 4, 2}), 1u);
}

TEST_F(RolloutSwitchingOptionChooserTest, InARolloutTheRolloutsChooserChoosesFromTheKeys) {
    ON_CALL(is_in_rollout, is_in_rollout()).WillByDefault(Return(true));
    EXPECT_CALL(choose_in_tree, choose(_)).Times(0);
    EXPECT_CALL(choose_in_rollout, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_rollout, choose(std::vector<size_t>{0, 4, 2})).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({0, 4, 2}), 2u);
}

TEST_F(RolloutSwitchingOptionChooserTest, EachChoiceAsksWhetherARolloutIsUnderWay) {
    EXPECT_CALL(is_in_rollout, is_in_rollout()).WillOnce(Return(false)).WillOnce(Return(true));
    ON_CALL(choose_in_tree, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_in_rollout, choose(_)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({0, 1}), 0u);
    EXPECT_EQ(chooser.choose({0, 1}), 1u);
}
