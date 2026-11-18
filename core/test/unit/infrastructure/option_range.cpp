#include <gtest/gtest.h>
#include "infrastructure/option_range.hpp"

namespace {

struct OptionRangeTest : public ::testing::Test {
    option_range options{3};
};

}

TEST_F(OptionRangeTest, ItsSizeIsTheOptionCount) {
    EXPECT_EQ(options.size(), 3u);
}

TEST_F(OptionRangeTest, EachPositionHoldsItsOwnIndex) {
    EXPECT_EQ(options.at(0), 0u);
    EXPECT_EQ(options.at(2), 2u);
}
