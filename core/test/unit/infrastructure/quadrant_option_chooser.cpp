#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quadrant_option_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseQuadrant {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseInQuadrant {
    MOCK_METHOD(size_t, choose, (size_t));
};

// Keys: 3 = made and fresh, 2 = given and fresh, 1 = made and consumed, 0 = given and consumed.
struct QuadrantOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseQuadrant> choose_quadrant;
    NiceMock<MockChooseInQuadrant> choose_in_quadrant;
    quadrant_option_chooser<NiceMock<MockChooseQuadrant>, NiceMock<MockChooseInQuadrant>> chooser{
        choose_quadrant, choose_in_quadrant};

    void SetUp() override {
        ON_CALL(choose_in_quadrant, choose(_)).WillByDefault(Return(0));
    }
};

}

TEST_F(QuadrantOptionChooserTest, TheQuadrantIsChosenAmongThoseHoldingOptionsThenTheOptionWithinIt) {
    EXPECT_CALL(choose_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_quadrant, choose(3)).WillOnce(Return(1));
    EXPECT_CALL(choose_in_quadrant, choose(2)).WillOnce(Return(1));
    // Quadrants present: made and fresh, given and fresh (positions 1 and 4), given and consumed.
    EXPECT_EQ(chooser.choose({3, 2, 0, 2}), 3u);
}

TEST_F(QuadrantOptionChooserTest, QuadrantsAreOfferedMadeAndFreshFirstGivenAndConsumedLast) {
    ON_CALL(choose_quadrant, choose(4)).WillByDefault(Return(0));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 2u);
    ON_CALL(choose_quadrant, choose(4)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 1u);
    ON_CALL(choose_quadrant, choose(4)).WillByDefault(Return(2));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 0u);
    ON_CALL(choose_quadrant, choose(4)).WillByDefault(Return(3));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 3u);
}

TEST_F(QuadrantOptionChooserTest, EmptyQuadrantsAreNotOffered) {
    EXPECT_CALL(choose_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_quadrant, choose(2)).WillOnce(Return(1));
    // Only made and consumed, and given and consumed, hold options.
    EXPECT_EQ(chooser.choose({1, 0}), 1u);
}

TEST_F(QuadrantOptionChooserTest, OneQuadrantAsksOnlyWithinIt) {
    EXPECT_CALL(choose_quadrant, choose(_)).Times(0);
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({2, 2, 2}), 2u);
}

TEST_F(QuadrantOptionChooserTest, AQuadrantOfOneOptionMakesNoChoiceWithinIt) {
    ON_CALL(choose_quadrant, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({2, 3, 2}), 1u);
}

TEST_F(QuadrantOptionChooserTest, ASingleOptionIsTakenWithoutAnyChoice) {
    EXPECT_CALL(choose_quadrant, choose(_)).Times(0);
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({1}), 0u);
}
