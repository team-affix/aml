#include <gtest/gtest.h>
#include "infrastructure/uniform_value_finder.hpp"

namespace {

struct UniformValueFinderTest : public ::testing::Test {
    UniformValueFinderTest();
    value falsity;
    value truth;
    uniform_value_finder finder;
};

UniformValueFinderTest::UniformValueFinderTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , finder() {}

}

TEST_F(UniformValueFinderTest, AColumnHoldingOneValueGivesThatValue) {
    EXPECT_EQ(finder.find(column{&truth, &truth, &truth}), std::optional<const value*>(&truth));
}

TEST_F(UniformValueFinderTest, AColumnHoldingTwoValuesGivesNone) {
    EXPECT_FALSE(finder.find(column{&truth, &falsity, &truth}).has_value());
}

TEST_F(UniformValueFinderTest, AnEmptyColumnGivesNone) {
    EXPECT_FALSE(finder.find(column{}).has_value());
}

TEST_F(UniformValueFinderTest, ASingleCellGivesItsValue) {
    EXPECT_EQ(finder.find(column{&falsity}), std::optional<const value*>(&falsity));
}

TEST_F(UniformValueFinderTest, ADifferenceInTheLastCellGivesNone) {
    EXPECT_FALSE(finder.find(column{&truth, &truth, &truth, &falsity}).has_value());
}

TEST_F(UniformValueFinderTest, ValuesAreComparedByPointer) {
    const value other_truth{.tag = 1, .args = {}};
    EXPECT_FALSE(finder.find(column{&truth, &other_truth}).has_value());
}
