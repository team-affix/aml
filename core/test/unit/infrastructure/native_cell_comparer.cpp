#include <any>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/native_cell_comparer.hpp"

using namespace AML;

namespace {

struct NativeCellComparerTest : public ::testing::Test {
    NativeCellComparerTest();
    std::any left;
    std::any right;
    native_cell_comparer<int> comparer;
};

NativeCellComparerTest::NativeCellComparerTest()
    : left(std::vector<int>{3, 7})
    , right(std::vector<int>{7})
    , comparer() {}

}

TEST_F(NativeCellComparerTest, EqualCellsCompareEqual) {
    EXPECT_TRUE(comparer(left, 1, right, 0));
}

TEST_F(NativeCellComparerTest, DifferentCellsCompareUnequal) {
    EXPECT_FALSE(comparer(left, 0, right, 0));
}

TEST_F(NativeCellComparerTest, RowsOfOneColumnCanBeCompared) {
    EXPECT_FALSE(comparer(left, 0, left, 1));
    EXPECT_TRUE(comparer(left, 1, left, 1));
}

TEST_F(NativeCellComparerTest, BooleanCellsAreReadFromTheirPackedVector) {
    const native_cell_comparer<bool> booleans;
    EXPECT_TRUE(booleans(std::any(std::vector<bool>{false, true}), 1, std::any(std::vector<bool>{true}), 0));
}
