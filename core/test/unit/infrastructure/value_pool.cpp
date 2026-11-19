#include <gtest/gtest.h>
#include "infrastructure/value_pool.hpp"

namespace {

struct ValuePoolTest : public ::testing::Test {
    value_pool values;
};

}

TEST_F(ValuePoolTest, EqualValuesShareOnePointer) {
    const value* zero = values.make(0, {});
    EXPECT_EQ(values.make(1, {zero}), values.make(1, {values.make(0, {})}));
}

TEST_F(ValuePoolTest, DifferentValuesGetDifferentPointers) {
    const value* zero = values.make(0, {});
    EXPECT_NE(values.make(1, {zero}), zero);
}

TEST_F(ValuePoolTest, AMadeValueHoldsItsConstructorAndArguments) {
    const value* zero = values.make(0, {});
    EXPECT_EQ(*values.make(1, {zero}), (value{.ctor = 1, .args = {zero}}));
}

TEST_F(ValuePoolTest, TheSameConstructorWithDifferentArgumentsGivesDifferentValues) {
    const value* zero = values.make(0, {});
    const value* one = values.make(1, {zero});
    EXPECT_NE(values.make(1, {one}), one);
}

TEST_F(ValuePoolTest, ArgumentOrderMatters) {
    const value* zero = values.make(0, {});
    const value* one = values.make(1, {zero});
    EXPECT_NE(values.make(2, {zero, one}), values.make(2, {one, zero}));
}

TEST_F(ValuePoolTest, PointersStayValidAsThePoolGrows) {
    const value* zero = values.make(0, {});
    const value* number = zero;
    for (size_t step = 0; step < 1000; ++step)
        number = values.make(1, {number});
    EXPECT_EQ(*zero, (value{.ctor = 0, .args = {}}));
    EXPECT_EQ(values.make(0, {}), zero);
}
