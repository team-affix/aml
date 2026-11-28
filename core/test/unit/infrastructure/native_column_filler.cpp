#include <any>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/native_column_filler.hpp"

using namespace AML;

TEST(NativeColumnFillerTest, EveryRowHoldsTheValue) {
    const native_column_filler<std::string> filler;
    EXPECT_EQ(std::any_cast<const std::vector<std::string>&>(filler(std::any(std::string("x")), 3)),
              (std::vector<std::string>{"x", "x", "x"}));
}

TEST(NativeColumnFillerTest, NoRowsGiveAnEmptyColumn) {
    const native_column_filler<int> filler;
    EXPECT_TRUE(std::any_cast<const std::vector<int>&>(filler(std::any(4), 0)).empty());
}
