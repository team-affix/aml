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

TEST_F(ValuePoolTest, AMadeValueHoldsItsConstructorAndFields) {
    const value* zero = values.make(0, {});
    EXPECT_EQ(*values.make(1, {zero}), (value{.ctor = 1, .fields = {zero}}));
}
