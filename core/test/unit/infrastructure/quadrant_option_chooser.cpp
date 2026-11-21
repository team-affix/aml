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

struct MockChooseOrigin {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseFreshness {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseInQuadrant {
    MOCK_METHOD(size_t, choose, (size_t));
};

// Keys: 3 = made and fresh, 2 = given and fresh, 1 = made and consumed, 0 = given and consumed.
struct QuadrantOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseOrigin> choose_origin;
    NiceMock<MockChooseFreshness> choose_freshness;
    NiceMock<MockChooseInQuadrant> choose_in_quadrant;
    quadrant_option_chooser<NiceMock<MockChooseOrigin>, NiceMock<MockChooseFreshness>, NiceMock<MockChooseInQuadrant>>
        chooser{choose_origin, choose_freshness, choose_in_quadrant};

    void SetUp() override {
        ON_CALL(choose_in_quadrant, choose(_)).WillByDefault(Return(0));
    }
};

}

TEST_F(QuadrantOptionChooserTest, OriginIsAskedThenFreshnessThenTheOptionWithinTheQuadrant) {
    EXPECT_CALL(choose_origin, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_freshness, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_origin, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_freshness, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_in_quadrant, choose(2)).WillOnce(Return(1));
    // Given and fresh holds positions 1 and 4.
    EXPECT_EQ(chooser.choose({3, 2, 0, 1, 2}), 4u);
}

TEST_F(QuadrantOptionChooserTest, TheFirstAnswersAreMadeAndFresh) {
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 2u);
}

TEST_F(QuadrantOptionChooserTest, TheSecondAnswersAreGivenAndConsumed) {
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(1));
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({1, 2, 3, 0}), 3u);
}

TEST_F(QuadrantOptionChooserTest, OnlyGivenOptionsAskNoOriginQuestion) {
    EXPECT_CALL(choose_origin, choose(_)).Times(0);
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(1));
    EXPECT_EQ(chooser.choose({2, 0}), 1u);
}

TEST_F(QuadrantOptionChooserTest, OneFreshnessOnTheChosenSideAsksNoFreshnessQuestion) {
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_freshness, choose(_)).Times(0);
    // Made options are all consumed.
    EXPECT_EQ(chooser.choose({2, 1, 0}), 1u);
}

TEST_F(QuadrantOptionChooserTest, AQuadrantOfOneOptionMakesNoChoiceWithinIt) {
    ON_CALL(choose_origin, choose(_)).WillByDefault(Return(0));
    ON_CALL(choose_freshness, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({2, 3, 2}), 1u);
}

TEST_F(QuadrantOptionChooserTest, OneQuadrantAsksOnlyWithinIt) {
    EXPECT_CALL(choose_origin, choose(_)).Times(0);
    EXPECT_CALL(choose_freshness, choose(_)).Times(0);
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_quadrant, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({2, 2, 2}), 2u);
}

TEST_F(QuadrantOptionChooserTest, ASingleOptionIsTakenWithoutAnyChoice) {
    EXPECT_CALL(choose_origin, choose(_)).Times(0);
    EXPECT_CALL(choose_freshness, choose(_)).Times(0);
    EXPECT_CALL(choose_in_quadrant, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({1}), 0u);
}
