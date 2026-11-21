#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/uniform_value_finder.hpp"

using namespace AML;

namespace {

struct UniformValueFinderTest : public ::testing::Test {
    UniformValueFinderTest();
    column_view view(column cells);
    value falsity;
    value truth;
    uniform_value_finder finder;
};

UniformValueFinderTest::UniformValueFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , finder() {}

column_view UniformValueFinderTest::view(column cells) {
    return column_view::of(typed_column{.type = 0, .cells = std::move(cells)});
}

}

TEST_F(UniformValueFinderTest, AColumnHoldingOneValueGivesThatValue) {
    const column_view cells = view({truth, truth, truth});
    const std::optional<const value*> found = finder.find(cells);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found.value(), truth);
}

TEST_F(UniformValueFinderTest, AColumnHoldingTwoValuesGivesNone) {
    EXPECT_FALSE(finder.find(view({truth, falsity, truth})).has_value());
}

TEST_F(UniformValueFinderTest, AnEmptyColumnGivesNone) {
    EXPECT_FALSE(finder.find(view({})).has_value());
}

TEST_F(UniformValueFinderTest, ASingleCellGivesItsValue) {
    const column_view cells = view({falsity});
    const std::optional<const value*> found = finder.find(cells);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found.value(), falsity);
}

TEST_F(UniformValueFinderTest, ADifferenceInTheLastCellGivesNone) {
    EXPECT_FALSE(finder.find(view({truth, truth, truth, falsity})).has_value());
}

TEST_F(UniformValueFinderTest, ValuesAreComparedByStructure) {
    const value other_truth{.ctor = 1, .args = {}};
    EXPECT_TRUE(finder.find(view({truth, other_truth})).has_value());
    const value nested{.ctor = 2, .args = {truth}};
    const value other_nested{.ctor = 2, .args = {falsity}};
    EXPECT_FALSE(finder.find(view({nested, other_nested})).has_value());
}

TEST_F(UniformValueFinderTest, TheFoundValueIsTheFirstSelectedCell) {
    const column_view cells{
        .type = 0,
        .cells = std::make_shared<const column>(column{falsity, truth, truth}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{2, 1})};
    EXPECT_EQ(finder.find(cells), std::optional<const value*>(&(*cells.cells)[2]));
}

TEST_F(UniformValueFinderTest, OnlyTheSelectedRowsCount) {
    const column_view cells{
        .type = 0,
        .cells = std::make_shared<const column>(column{falsity, truth, truth}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{1, 2})};
    EXPECT_TRUE(finder.find(cells).has_value());
}

TEST_F(UniformValueFinderTest, ASelectionOfNoRowsGivesNone) {
    const column_view cells{
        .type = 0,
        .cells = std::make_shared<const column>(column{truth}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{})};
    EXPECT_FALSE(finder.find(cells).has_value());
}
