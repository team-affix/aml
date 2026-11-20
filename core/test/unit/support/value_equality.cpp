#include <vector>
#include <gtest/gtest.h>
#include "support/value_equality.hpp"

using namespace AML;

namespace {

// Booleans answer: false is constructor 3 and true is 7. The compared values are naturals: zero 0, suc 1.
struct ValueEqualityTest : public ::testing::Test {
    ValueEqualityTest();
    ctor_id false_ctor;
    ctor_id true_ctor;
    value falsity;
    value truth;
    value zero;
    value one;
    value two;
    value_equality primitive;
};

ValueEqualityTest::ValueEqualityTest()
    : false_ctor(3)
    , true_ctor(7)
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , zero{.ctor = 0, .args = {}}
    , one{.ctor = 1, .args = {zero}}
    , two{.ctor = 1, .args = {one}}
    , primitive(false_ctor, true_ctor) {}

}

TEST_F(ValueEqualityTest, TheSameValueIsEqual) {
    EXPECT_EQ(primitive({&two, &two}), truth);
    EXPECT_EQ(primitive({&zero, &zero}), truth);
}

TEST_F(ValueEqualityTest, DifferentValuesAreNotEqual) {
    EXPECT_EQ(primitive({&one, &two}), falsity);
    EXPECT_EQ(primitive({&zero, &one}), falsity);
}

TEST_F(ValueEqualityTest, EqualityIsByStructureNotByObject) {
    const value another_two{.ctor = 1, .args = {value{.ctor = 1, .args = {value{.ctor = 0, .args = {}}}}}};
    EXPECT_EQ(primitive({&two, &another_two}), truth);
}

TEST_F(ValueEqualityTest, EqualityIsSymmetric) {
    EXPECT_EQ(primitive({&one, &two}), primitive({&two, &one}));
    EXPECT_EQ(primitive({&two, &two}), primitive({&two, &two}));
}
