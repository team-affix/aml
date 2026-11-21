#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/weighted_quadrant_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseFreshness {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseOrigin {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseInQuadrant {
    MOCK_METHOD(size_t, choose, (size_t));
};

// Keys: 3 = made and fresh, 2 = given and fresh, 1 = made and consumed, 0 = given and consumed.
struct WeightedQuadrantChooserTest : public ::testing::Test {
    NiceMock<MockChooseFreshness> choose_freshness;
    NiceMock<MockChooseOrigin> choose_origin;
    NiceMock<MockChooseInQuadrant> choose_in_quadrant;
    weighted_quadrant_chooser<
        NiceMock<MockChooseFreshness>, NiceMock<MockChooseOrigin>, NiceMock<MockChooseInQuadrant>>
        chooser{choose_freshness, choose_origin, choose_in_quadrant};

    void SetUp() override {
        ON_CALL(choose_in_quadrant, choose(_)).WillByDefault(Return(0));
    }
};

}

TEST_F(WeightedQuadrantChooserTest, ThreeOfTheFourFreshnessAnswersMeanFresh) {
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    for (size_t answer = 0; answer < 3; ++answer) {
        ON_CALL(choose_freshness, choose(4)).WillByDefault(Return(answer));
        EXPECT_EQ(chooser.choose({1, 3, 0, 2}), 1u);
    }
    ON_CALL(choose_freshness, choose(4)).WillByDefault(Return(3));
    EXPECT_EQ(chooser.choose({1, 3, 0, 2}), 0u);
}

TEST_F(WeightedQuadrantChooserTest, TheFirstOriginAnswerIsMadeAndTheSecondGiven) {
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_origin, choose(2)).WillByDefault(Return(0));
    EXPECT_EQ(chooser.choose({1, 3, 0, 2}), 1u);
    ON_CALL(choose_origin, choose(2)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({1, 3, 0, 2}), 3u);
}

TEST_F(WeightedQuadrantChooserTest, OnlyFreshOptionsAskNoFreshnessQuestion) {
    EXPECT_CALL(choose_freshness, choose(_)).Times(0);
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({3, 2}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, OnlyConsumedOptionsAskNoFreshnessQuestion) {
    EXPECT_CALL(choose_freshness, choose(_)).Times(0);
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    EXPECT_EQ(chooser.choose({0, 1}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, OneOriginOnTheChosenSideAsksNoOriginQuestion) {
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_origin, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({0, 2, 1}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, AQuadrantsOptionsAreChosenAmongInTheirGivenOrder) {
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(1));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({2, 3, 2, 0, 2}), 4u);
}

TEST_F(WeightedQuadrantChooserTest, AQuadrantOfOneOptionMakesNoChoiceWithinIt) {
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({2, 3, 2, 0}), 1u);
}
