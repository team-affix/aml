#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/tiered_option_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseTier {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseInTier {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct TieredOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseTier> choose_tier;
    NiceMock<MockChooseInTier> choose_in_tier;
    tiered_option_chooser<NiceMock<MockChooseTier>, NiceMock<MockChooseInTier>> chooser{
        choose_tier, choose_in_tier};
};

}

TEST_F(TieredOptionChooserTest, OneOrderMakesNoTierChoice) {
    EXPECT_CALL(choose_tier, choose(_)).Times(0);
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({0, 0, 0}), 2u);
}

TEST_F(TieredOptionChooserTest, TheTierChoiceIsTwoWay) {
    EXPECT_CALL(choose_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_tier, choose(2)).WillOnce(Return(0));
    chooser.choose({0, 1});
}

TEST_F(TieredOptionChooserTest, TakingTheHighestOrderChoosesAmongItsOptionsInOrder) {
    ON_CALL(choose_tier, choose(2)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose({0, 1, 0, 1}), 3u);
}

TEST_F(TieredOptionChooserTest, DescendingChoosesAmongTheNextLowerOrder) {
    ON_CALL(choose_tier, choose(2)).WillByDefault(Return(1));
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose({0, 1, 0, 1}), 2u);
}

TEST_F(TieredOptionChooserTest, TheLowestOrderIsTakenWithoutAFurtherChoice) {
    EXPECT_CALL(choose_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_tier, choose(2)).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(choose_in_tier, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({2, 1, 0}), 2u);
}

TEST_F(TieredOptionChooserTest, AMiddleOrderIsTakenAfterOneDescent) {
    EXPECT_CALL(choose_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_tier, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_tier, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_in_tier, choose(2)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({0, 2, 1, 1}), 2u);
}

TEST_F(TieredOptionChooserTest, OrdersWithNoOptionsAreSkipped) {
    EXPECT_CALL(choose_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_tier, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_in_tier, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({5, 0}), 1u);
}

TEST_F(TieredOptionChooserTest, ALoneOptionInItsOrderIsStillChosenThrough) {
    ON_CALL(choose_tier, choose(2)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({0, 0, 1}), 2u);
}

TEST_F(TieredOptionChooserTest, ASingleOptionIsChosenWithoutATierChoice) {
    EXPECT_CALL(choose_tier, choose(_)).Times(0);
    EXPECT_CALL(choose_in_tier, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_tier, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose({4}), 0u);
}

TEST_F(TieredOptionChooserTest, NoOptionsIsRejected) {
    EXPECT_THROW(chooser.choose({}), std::logic_error);
}
