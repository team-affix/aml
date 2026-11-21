#include <gtest/gtest.h>
#include "value_objects/bin_view.hpp"

using namespace AML;

namespace {

struct BinViewTest : public ::testing::Test {
    BinViewTest();
    value falsity;
    value truth;
    bin dense;
};

BinViewTest::BinViewTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , dense{
          .inputs = {typed_column{.type = 0, .cells = {falsity, truth}},
                     typed_column{.type = 1, .cells = {truth, truth}}},
          .labels = typed_column{.type = 0, .cells = {truth, falsity}}} {}

}

TEST_F(BinViewTest, OfViewsEveryInputAndTheLabels) {
    const bin_view viewed = bin_view::of(dense);
    ASSERT_EQ(viewed.inputs.size(), 2u);
    EXPECT_EQ(viewed.inputs[0], column_view::of(dense.inputs[0]));
    EXPECT_EQ(viewed.inputs[1], column_view::of(dense.inputs[1]));
    EXPECT_EQ(viewed.labels, column_view::of(dense.labels));
}

TEST_F(BinViewTest, OfSharesOneSelectionOfRowsAcrossTheColumns) {
    const bin_view viewed = bin_view::of(dense);
    EXPECT_EQ(viewed.inputs[0].rows, viewed.labels.rows);
    EXPECT_EQ(viewed.inputs[1].rows, viewed.labels.rows);
    EXPECT_EQ(*viewed.labels.rows, (std::vector<size_t>{0, 1}));
}

TEST_F(BinViewTest, OfABinWithoutInputsViewsItsLabels) {
    const bin_view viewed = bin_view::of(bin{.inputs = {}, .labels = typed_column{.type = 0, .cells = {truth}}});
    EXPECT_TRUE(viewed.inputs.empty());
    EXPECT_EQ(viewed.labels, column_view::of(typed_column{.type = 0, .cells = {truth}}));
}

TEST_F(BinViewTest, BinsAreEqualWhenTheirColumnsReadEqualValues) {
    EXPECT_EQ(bin_view::of(dense), bin_view::of(dense));
    bin other = dense;
    other.labels.cells[0] = falsity;
    EXPECT_NE(bin_view::of(dense), bin_view::of(other));
}
