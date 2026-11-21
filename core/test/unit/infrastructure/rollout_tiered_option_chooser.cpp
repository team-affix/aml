#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rollout_tiered_option_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockIsInRollout {
    MOCK_METHOD(bool, is_in_rollout, (), (const));
};

struct MockChooseFlat {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseByTier {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct RolloutTieredOptionChooserTest : public ::testing::Test {
    NiceMock<MockIsInRollout> is_in_rollout;
    NiceMock<MockChooseFlat> choose_flat;
    NiceMock<MockChooseByTier> choose_by_tier;
    rollout_tiered_option_chooser<NiceMock<MockIsInRollout>, NiceMock<MockChooseFlat>, NiceMock<MockChooseByTier>>
        chooser{is_in_rollout, choose_flat, choose_by_tier};
};

}

TEST_F(RolloutTieredOptionChooserTest, InTheTreeOneChoiceIsMadeAmongAllTheOptions) {
    ON_CALL(is_in_rollout, is_in_rollout()).WillByDefault(Return(false));
    EXPECT_CALL(choose_by_tier, choose(_)).Times(0);
    EXPECT_CALL(choose_flat, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_flat, choose(4)).WillOnce(Return(3));
    EXPECT_EQ(chooser.choose({0, 2, 1, 2}), 3u);
}

TEST_F(RolloutTieredOptionChooserTest, InARolloutTheOptionsAreChosenByTheirOrders) {
    ON_CALL(is_in_rollout, is_in_rollout()).WillByDefault(Return(true));
    EXPECT_CALL(choose_flat, choose(_)).Times(0);
    EXPECT_CALL(choose_by_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_by_tier, choose(std::vector<size_t>{0, 2, 1, 2})).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose({0, 2, 1, 2}), 1u);
}

TEST_F(RolloutTieredOptionChooserTest, EachChoiceAsksWhetherARolloutIsUnderWay) {
    EXPECT_CALL(is_in_rollout, is_in_rollout()).WillOnce(Return(false)).WillOnce(Return(true));
    ON_CALL(choose_flat, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_by_tier, choose(_)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({0, 1}), 0u);
    EXPECT_EQ(chooser.choose({0, 1}), 1u);
}

TEST_F(RolloutTieredOptionChooserTest, ASingleOptionIsStillChosenThrough) {
    ON_CALL(is_in_rollout, is_in_rollout()).WillByDefault(Return(false));
    EXPECT_CALL(choose_flat, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_flat, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({5}), 0u);
}
