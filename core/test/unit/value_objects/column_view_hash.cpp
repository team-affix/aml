#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "value_objects/column_view_hash.hpp"

using namespace AML;

namespace {

struct ColumnViewHashTest : public ::testing::Test {
    ColumnViewHashTest();
    value falsity;
    value truth;
    column_view_hash hasher;
};

ColumnViewHashTest::ColumnViewHashTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , hasher() {}

}

TEST_F(ColumnViewHashTest, EqualTypedColumnsHashEqual) {
    EXPECT_EQ(hasher(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}})),
              hasher(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}})));
}

TEST_F(ColumnViewHashTest, CellOrderChangesTheHash) {
    EXPECT_NE(hasher(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}})),
              hasher(column_view::of(typed_column{.type = 0, .cells = {truth, falsity}})));
}

TEST_F(ColumnViewHashTest, LengthChangesTheHash) {
    EXPECT_NE(hasher(column_view::of(typed_column{.type = 0, .cells = {falsity}})),
              hasher(column_view::of(typed_column{.type = 0, .cells = {falsity, falsity}})));
}

TEST_F(ColumnViewHashTest, TheTypeChangesTheHash) {
    EXPECT_NE(hasher(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}})),
              hasher(column_view::of(typed_column{.type = 1, .cells = {falsity, truth}})));
}

TEST_F(ColumnViewHashTest, EmptyColumnsOfOneTypeHashEqual) {
    EXPECT_EQ(hasher(column_view::of(typed_column{.type = 2, .cells = {}})),
              hasher(column_view::of(typed_column{.type = 2, .cells = {}})));
}

TEST_F(ColumnViewHashTest, DifferentValuesHashApart) {
    EXPECT_NE(hasher(column_view::of(typed_column{.type = 0, .cells = {truth}})),
              hasher(column_view::of(typed_column{.type = 0, .cells = {falsity}})));
}

TEST_F(ColumnViewHashTest, CellsHashByStructure) {
    const value pair{.ctor = 4, .args = {truth, falsity}};
    const value same_pair{.ctor = 4, .args = {value{.ctor = 1, .args = {}}, value{.ctor = 0, .args = {}}}};
    EXPECT_EQ(hasher(column_view::of(typed_column{.type = 3, .cells = {pair}})),
              hasher(column_view::of(typed_column{.type = 3, .cells = {same_pair}})));
}

TEST_F(ColumnViewHashTest, ViewsReadingEqualValuesHashEqualWhateverTheirRows) {
    const column_view whole = column_view::of(typed_column{.type = 0, .cells = {truth, falsity}});
    const column_view selected{
        .type = 0,
        .cells = std::make_shared<const column>(column{falsity, truth, truth, falsity}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{2, 3})};
    EXPECT_EQ(hasher(whole), hasher(selected));
}

TEST_F(ColumnViewHashTest, OnlyTheSelectedRowsCount) {
    const column_view first = column_view{
        .type = 0,
        .cells = std::make_shared<const column>(column{truth, falsity}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{0})};
    const column_view second = column_view{
        .type = 0,
        .cells = std::make_shared<const column>(column{truth, truth}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{0})};
    EXPECT_EQ(hasher(first), hasher(second));
}
