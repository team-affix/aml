#include <any>
#include <functional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/native_column_hasher.hpp"

using namespace AML;

namespace {

struct NativeColumnHasherTest : public ::testing::Test {
    native_column_hasher<int, std::hash<int>> hasher;
};

}

TEST_F(NativeColumnHasherTest, EqualColumnsHashEqual) {
    EXPECT_EQ(hasher(std::any(std::vector<int>{1, 2, 3})), hasher(std::any(std::vector<int>{1, 2, 3})));
}

TEST_F(NativeColumnHasherTest, OrderChangesTheHash) {
    EXPECT_NE(hasher(std::any(std::vector<int>{1, 2})), hasher(std::any(std::vector<int>{2, 1})));
}

TEST_F(NativeColumnHasherTest, AnExtraCellChangesTheHash) {
    EXPECT_NE(hasher(std::any(std::vector<int>{0})), hasher(std::any(std::vector<int>{0, 0})));
}

TEST_F(NativeColumnHasherTest, ADifferentCellChangesTheHash) {
    EXPECT_NE(hasher(std::any(std::vector<int>{1, 2})), hasher(std::any(std::vector<int>{1, 3})));
}
