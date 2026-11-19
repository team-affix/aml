#include <gtest/gtest.h>
#include "infrastructure/value_pool.hpp"

namespace {

// Nat is type 0 {zero, suc}; a pair is type 1 {pair}; bool is type 2 {false, true}.
struct ValuePoolTest : public ::testing::Test {
    ValuePoolTest();
    ctor_ref zero_ctor;
    ctor_ref suc_ctor;
    ctor_ref pair_ctor;
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    value_pool values;
};

ValuePoolTest::ValuePoolTest()
    : zero_ctor{.type = 0, .tag = 0}
    , suc_ctor{.type = 0, .tag = 1}
    , pair_ctor{.type = 1, .tag = 0}
    , false_ctor{.type = 2, .tag = 0}
    , true_ctor{.type = 2, .tag = 1}
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
    EXPECT_EQ(*values.make(suc_ctor, {zero}), (value{.ctor = suc_ctor, .args = {zero}}));
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

TEST_F(ValuePoolTest, TheSameTagOfDifferentTypesGivesDifferentValues) {
    EXPECT_NE(values.make(zero_ctor, {}), values.make(false_ctor, {}));
    EXPECT_NE(values.make(suc_ctor, {}), values.make(true_ctor, {}));
}

TEST_F(ValuePoolTest, PointersStayValidAsThePoolGrows) {
    const value* zero = values.make(zero_ctor, {});
    const value* number = zero;
    for (size_t step = 0; step < 1000; ++step)
        number = values.make(suc_ctor, {number});
    EXPECT_EQ(*zero, (value{.ctor = zero_ctor, .args = {}}));
    EXPECT_EQ(values.make(zero_ctor, {}), zero);
}
