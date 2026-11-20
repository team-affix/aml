#include <vector>
#include <gtest/gtest.h>
#include "support/bool_not.hpp"

using namespace AML;

namespace {

// The user boolean type's constructors are ids 3 (false) and 7 (true), so nothing depends on them being 0 and 1.
struct BoolNotTest : public ::testing::Test {
    BoolNotTest();
    ctor_id false_ctor;
    ctor_id true_ctor;
    value falsity;
    value truth;
    bool_not primitive;
};

BoolNotTest::BoolNotTest()
    : false_ctor(3)
    , true_ctor(7)
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , primitive(false_ctor, true_ctor) {}

}

TEST_F(BoolNotTest, TrueBecomesFalseAndFalseBecomesTrue) {
    EXPECT_EQ(primitive({&truth}), falsity);
    EXPECT_EQ(primitive({&falsity}), truth);
}

TEST_F(BoolNotTest, NegatingTwiceGivesTheOriginal) {
    const value once = primitive({&truth});
    EXPECT_EQ(primitive({&once}), truth);
}

TEST_F(BoolNotTest, AnyConstructorButTrueCountsAsFalse) {
    const value other{.ctor = 9, .args = {}};
    EXPECT_EQ(primitive({&other}), truth);
}
