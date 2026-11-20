#include <gtest/gtest.h>
#include "infrastructure/uniform_value_finder.hpp"

using namespace AML;

namespace {

struct UniformValueFinderTest : public ::testing::Test {
    UniformValueFinderTest();
    value falsity;
    value truth;
    uniform_value_finder finder;
};

UniformValueFinderTest::UniformValueFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , finder() {}

}

TEST_F(UniformValueFinderTest, AColumnHoldingOneValueGivesThatValue) {
    const column cells{truth, truth, truth};
    const std::optional<const value*> found = finder.find(cells);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found.value(), truth);
}

TEST_F(UniformValueFinderTest, AColumnHoldingTwoValuesGivesNone) {
    EXPECT_FALSE(finder.find(column{truth, falsity, truth}).has_value());
}

TEST_F(UniformValueFinderTest, AnEmptyColumnGivesNone) {
    EXPECT_FALSE(finder.find(column{}).has_value());
}

TEST_F(UniformValueFinderTest, ASingleCellGivesItsValue) {
    const column cells{falsity};
    const std::optional<const value*> found = finder.find(cells);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found.value(), falsity);
}

TEST_F(UniformValueFinderTest, ADifferenceInTheLastCellGivesNone) {
    EXPECT_FALSE(finder.find(column{truth, truth, truth, falsity}).has_value());
}

TEST_F(UniformValueFinderTest, ValuesAreComparedByStructure) {
    const value other_truth{.ctor = 1, .args = {}};
    EXPECT_TRUE(finder.find(column{truth, other_truth}).has_value());
    const value nested{.ctor = 2, .args = {truth}};
    const value other_nested{.ctor = 2, .args = {falsity}};
    EXPECT_FALSE(finder.find(column{nested, other_nested}).has_value());
}

TEST_F(UniformValueFinderTest, TheFoundValueIsTheColumnsFirstCell) {
    const column cells{truth, truth};
    EXPECT_EQ(finder.find(cells), std::optional<const value*>(&cells.front()));
}
