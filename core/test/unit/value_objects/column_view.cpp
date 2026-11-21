#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "value_objects/column_view.hpp"

using namespace AML;

namespace {

struct ColumnViewTest : public ::testing::Test {
    ColumnViewTest();
    column_view select(type_id type, column cells, std::vector<size_t> rows);
    value falsity;
    value truth;
};

ColumnViewTest::ColumnViewTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}} {}

column_view ColumnViewTest::select(type_id type, column cells, std::vector<size_t> rows) {
    return column_view{
        .type = type,
        .cells = std::make_shared<const column>(std::move(cells)),
        .rows = std::make_shared<const std::vector<size_t>>(std::move(rows))};
}

}

TEST_F(ColumnViewTest, OfViewsEveryCellInOrder) {
    const column_view whole = column_view::of(typed_column{.type = 3, .cells = {truth, falsity, truth}});
    EXPECT_EQ(whole.type, 3u);
    EXPECT_EQ(*whole.cells, (column{truth, falsity, truth}));
    EXPECT_EQ(*whole.rows, (std::vector<size_t>{0, 1, 2}));
}

TEST_F(ColumnViewTest, OfAnEmptyColumnSelectsNoRows) {
    const column_view empty = column_view::of(typed_column{.type = 0, .cells = {}});
    EXPECT_TRUE(empty.cells->empty());
    EXPECT_TRUE(empty.rows->empty());
}

TEST_F(ColumnViewTest, ViewsReadingTheSameValuesAreEqualWhateverTheirCellsAndRows) {
    EXPECT_EQ(column_view::of(typed_column{.type = 0, .cells = {truth, falsity}}),
              select(0, {falsity, truth, truth, falsity}, {1, 3}));
}

TEST_F(ColumnViewTest, TheRowsOrderTheValues) {
    EXPECT_NE(select(0, {truth, falsity}, {0, 1}), select(0, {truth, falsity}, {1, 0}));
}

TEST_F(ColumnViewTest, OnlyTheSelectedRowsAreCompared) {
    EXPECT_EQ(select(0, {truth, falsity}, {0}), select(0, {truth, truth}, {1}));
}

TEST_F(ColumnViewTest, TheTypeTellsEqualValuesApart) {
    EXPECT_NE(column_view::of(typed_column{.type = 0, .cells = {truth}}),
              column_view::of(typed_column{.type = 1, .cells = {truth}}));
}

TEST_F(ColumnViewTest, TheNumberOfRowsTellsViewsApart) {
    EXPECT_NE(select(0, {truth, truth}, {0}), select(0, {truth, truth}, {0, 1}));
}

TEST_F(ColumnViewTest, EmptyViewsOfOneTypeAreEqual) {
    EXPECT_EQ(select(2, {truth}, {}), column_view::of(typed_column{.type = 2, .cells = {}}));
}

TEST_F(ColumnViewTest, CellsCompareByStructure) {
    const value pair{.ctor = 4, .args = {truth, falsity}};
    const value same_pair{.ctor = 4, .args = {value{.ctor = 1, .args = {}}, value{.ctor = 0, .args = {}}}};
    EXPECT_EQ(column_view::of(typed_column{.type = 3, .cells = {pair}}),
              column_view::of(typed_column{.type = 3, .cells = {same_pair}}));
}
