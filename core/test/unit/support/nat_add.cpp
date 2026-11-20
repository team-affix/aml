#include <vector>
#include <gtest/gtest.h>
#include "support/nat_add.hpp"

using namespace AML;

namespace {

// Naturals: zero is constructor 4 and suc is 2, so nothing depends on them being 0 and 1.
struct NatAddTest : public ::testing::Test {
    NatAddTest();
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    value zero;
    value one;
    value two;
    value three;
    nat_add primitive;
};

NatAddTest::NatAddTest()
    : zero_ctor(4)
    , suc_ctor(2)
    , zero{.ctor = zero_ctor, .args = {}}
    , one{.ctor = suc_ctor, .args = {zero}}
    , two{.ctor = suc_ctor, .args = {one}}
    , three{.ctor = suc_ctor, .args = {two}}
    , primitive(suc_ctor) {}

}

TEST_F(NatAddTest, AddingZeroGivesTheLeftOperand) {
    EXPECT_EQ(primitive({&two, &zero}), two);
}

TEST_F(NatAddTest, EachSuccessorOfTheRightOperandWrapsTheLeftOperand) {
    EXPECT_EQ(primitive({&one, &two}), three);
    EXPECT_EQ(primitive({&zero, &one}), one);
}

TEST_F(NatAddTest, ZeroPlusZeroIsZero) {
    EXPECT_EQ(primitive({&zero, &zero}), zero);
}

TEST_F(NatAddTest, TheRightOperandOnlyCountsSuccessors) {
    EXPECT_EQ(primitive({&zero, &three}), three);
}

TEST_F(NatAddTest, TheOperandsAreLeftUnchanged) {
    const value before_one = one;
    const value before_two = two;
    primitive({&one, &two});
    EXPECT_EQ(one, before_one);
    EXPECT_EQ(two, before_two);
}
