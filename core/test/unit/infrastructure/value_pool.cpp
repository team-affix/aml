#include <gtest/gtest.h>
#include "infrastructure/value_pool.hpp"

using namespace AML;

namespace {

// Tags of nat {zero, suc}, a pair {pair} and bool {false, true}. Values carry no type, only tags.
struct ValuePoolTest : public ::testing::Test {
    ValuePoolTest();
    size_t zero_ctor;
    size_t suc_ctor;
    size_t pair_ctor;
    size_t false_ctor;
    size_t true_ctor;
    value_pool values;
};

ValuePoolTest::ValuePoolTest()
    : zero_ctor(0)
    , suc_ctor(1)
    , pair_ctor(0)
    , false_ctor(0)
    , true_ctor(1)
    , values() {}

}

TEST_F(ValuePoolTest, EqualValuesShareOnePointer) {
    const value* zero = values.make(zero_ctor, {});
    EXPECT_EQ(values.make(suc_ctor, {zero}), values.make(suc_ctor, {values.make(zero_ctor, {})}));
}

TEST_F(ValuePoolTest, DifferentValuesGetDifferentPointers) {
    const value* zero = values.make(zero_ctor, {});
    EXPECT_NE(values.make(suc_ctor, {zero}), zero);
}

TEST_F(ValuePoolTest, AMadeValueHoldsItsConstructorAndArguments) {
    const value* zero = values.make(zero_ctor, {});
    EXPECT_EQ(*values.make(suc_ctor, {zero}), (value{.tag = suc_ctor, .args = {zero}}));
}

TEST_F(ValuePoolTest, TheSameConstructorWithDifferentArgumentsGivesDifferentValues) {
    const value* one = values.make(suc_ctor, {values.make(zero_ctor, {})});
    EXPECT_NE(values.make(suc_ctor, {one}), one);
}

TEST_F(ValuePoolTest, ArgumentOrderMatters) {
    const value* zero = values.make(zero_ctor, {});
    const value* one = values.make(suc_ctor, {zero});
    EXPECT_NE(values.make(pair_ctor, {zero, one}), values.make(pair_ctor, {one, zero}));
}

TEST_F(ValuePoolTest, ValuesCarryNoTypeSoTheSameTagAndArgumentsAreOneValue) {
    EXPECT_EQ(values.make(zero_ctor, {}), values.make(false_ctor, {}));
}

TEST_F(ValuePoolTest, PointersStayValidAsThePoolGrows) {
    const value* zero = values.make(zero_ctor, {});
    const value* number = zero;
    for (size_t step = 0; step < 1000; ++step)
        number = values.make(suc_ctor, {number});
    EXPECT_EQ(*zero, (value{.tag = zero_ctor, .args = {}}));
    EXPECT_EQ(values.make(zero_ctor, {}), zero);
}
