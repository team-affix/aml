#include <gtest/gtest.h>
#include "infrastructure/option_range.hpp"

using namespace AML;

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

TEST_F(OptionRangeTest, ASingleOptionRangeHoldsZero) {
    const option_range single(1);
    EXPECT_EQ(single.size(), 1u);
    EXPECT_EQ(single.at(0), 0u);
}

TEST_F(OptionRangeTest, ALargeRangeNeedsNoStorage) {
    const option_range large(1000000);
    EXPECT_EQ(large.size(), 1000000u);
    EXPECT_EQ(large.at(999999), 999999u);
}
