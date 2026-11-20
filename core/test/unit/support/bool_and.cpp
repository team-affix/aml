#include <vector>
#include <gtest/gtest.h>
#include "support/bool_and.hpp"

using namespace AML;

namespace {

// The user boolean type's constructors are ids 3 (false) and 7 (true), so nothing depends on them being 0 and 1.
struct BoolAndTest : public ::testing::Test {
    BoolAndTest();
    ctor_id false_ctor;
    ctor_id true_ctor;
    value falsity;
    value truth;
    bool_and primitive;
};

BoolAndTest::BoolAndTest()
    : false_ctor(3)
    , true_ctor(7)
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , primitive(false_ctor, true_ctor) {}

}

TEST_F(BoolAndTest, OnlyTwoTruesGiveTrue) {
    EXPECT_EQ(primitive({&truth, &truth}), truth);
    EXPECT_EQ(primitive({&truth, &falsity}), falsity);
    EXPECT_EQ(primitive({&falsity, &truth}), falsity);
    EXPECT_EQ(primitive({&falsity, &falsity}), falsity);
}
