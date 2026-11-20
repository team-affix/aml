// A tiered_option_chooser over real binary_option_choosers: every choice it makes is a two-way choice, and the
// highest order is one two-way choice away however many lower-order options there are.

#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/binary_option_chooser.hpp"
#include "infrastructure/tiered_option_chooser.hpp"

using namespace AML;

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockChooseHalf {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct TieredChoiceIntegrationTest : public ::testing::Test {
    TieredChoiceIntegrationTest();
    StrictMock<MockChooseHalf> choose_half;
    binary_option_chooser<StrictMock<MockChooseHalf>> binary;
    tiered_option_chooser<
        binary_option_chooser<StrictMock<MockChooseHalf>>, binary_option_chooser<StrictMock<MockChooseHalf>>>
        tiered;
    std::vector<size_t> twelve_variables_and_one_derived;
};

TieredChoiceIntegrationTest::TieredChoiceIntegrationTest()
    : choose_half()
    , binary(choose_half)
    , tiered(binary, binary)
    , twelve_variables_and_one_derived{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1} {}

}

TEST_F(TieredChoiceIntegrationTest, ALoneHighestOrderFeatureIsOneTwoWayChoiceAway) {
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_EQ(tiered.choose(twelve_variables_and_one_derived), 12u);
}

TEST_F(TieredChoiceIntegrationTest, DescendingSplitsTheLowerOrderIntoTwoWayChoices) {
    InSequence order;
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(tiered.choose(twelve_variables_and_one_derived), 1u);
}

TEST_F(TieredChoiceIntegrationTest, AlwaysTakingTheFirstOptionTakesTheFirstOfTheHighestOrder) {
    EXPECT_CALL(choose_half, choose(2)).Times(3).WillRepeatedly(Return(0));
    EXPECT_EQ(tiered.choose({0, 2, 1, 2, 2}), 1u);
}
