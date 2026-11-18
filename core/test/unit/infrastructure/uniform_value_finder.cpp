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
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
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
