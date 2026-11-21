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

struct MockDrawFraction {
    MOCK_METHOD(double, draw, ());
};

struct MockChooseInQuadrant {
    MOCK_METHOD(size_t, choose, (size_t));
};

// Keys: 3 = made and fresh, 2 = given and fresh, 1 = made and consumed, 0 = given and consumed. Fresh weight 0.8,
// made weight 0.3.
struct WeightedQuadrantChooserTest : public ::testing::Test {
    NiceMock<MockDrawFraction> draw_fraction;
    NiceMock<MockChooseInQuadrant> choose_in_quadrant;
    weighted_quadrant_chooser<NiceMock<MockDrawFraction>, NiceMock<MockChooseInQuadrant>> chooser{
        draw_fraction, choose_in_quadrant, 0.8, 0.3};

    void SetUp() override {
        ON_CALL(choose_in_quadrant, choose(_)).WillByDefault(Return(0));
    }
};

}

TEST_F(WeightedQuadrantChooserTest, AFractionBelowTheFreshWeightIsFreshAndBelowTheMadeWeightMade) {
    EXPECT_CALL(draw_fraction, draw()).WillOnce(Return(0.79)).WillOnce(Return(0.29));
    EXPECT_EQ(chooser.choose({1, 2, 0, 3}), 3u);
}

TEST_F(WeightedQuadrantChooserTest, AFractionAtOrAboveAWeightTakesTheOtherSide) {
    EXPECT_CALL(draw_fraction, draw()).WillOnce(Return(0.8)).WillOnce(Return(0.3));
    EXPECT_EQ(chooser.choose({1, 2, 0, 3}), 2u);
}

TEST_F(WeightedQuadrantChooserTest, FreshnessIsDecidedBeforeOrigin) {
    EXPECT_CALL(draw_fraction, draw()).WillOnce(Return(0.9)).WillOnce(Return(0.1));
    EXPECT_EQ(chooser.choose({1, 2, 0, 3}), 0u);
}

TEST_F(WeightedQuadrantChooserTest, OnlyFreshOptionsDrawOnlyForOrigin) {
    EXPECT_CALL(draw_fraction, draw()).WillOnce(Return(0.5));
    EXPECT_EQ(chooser.choose({3, 2}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, OneOriginOnTheChosenSideDrawsOnlyForFreshness) {
    EXPECT_CALL(draw_fraction, draw()).WillOnce(Return(0.1));
    // The fresh side holds only given options.
    EXPECT_EQ(chooser.choose({1, 2, 0}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, OneQuadrantDrawsNothing) {
    EXPECT_CALL(draw_fraction, draw()).Times(0);
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({0, 0, 0}), 2u);
}

TEST_F(WeightedQuadrantChooserTest, AQuadrantsOptionsAreChosenAmongInTheirGivenOrder) {
    ON_CALL(draw_fraction, draw()).WillByDefault(Return(0.5));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(2)).WillOnce(Return(1));
    // Fresh (0.5 < 0.8), then given (0.5 >= 0.3): positions 1 and 3.
    EXPECT_EQ(chooser.choose({3, 2, 0, 2}), 3u);
}

TEST_F(WeightedQuadrantChooserTest, AQuadrantOfOneOptionMakesNoChoiceWithinIt) {
    ON_CALL(draw_fraction, draw()).WillByDefault(Return(0.1));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({0, 3, 2}), 1u);
}

TEST_F(WeightedQuadrantChooserTest, AWeightOfOneAlwaysTakesThatSide) {
    weighted_quadrant_chooser<NiceMock<MockDrawFraction>, NiceMock<MockChooseInQuadrant>> certain{
        draw_fraction, choose_in_quadrant, 1.0, 0.0};
    ON_CALL(draw_fraction, draw()).WillByDefault(Return(0.999999));
    EXPECT_EQ(certain.choose({3, 2, 1, 0}), 1u);
}
