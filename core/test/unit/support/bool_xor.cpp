#include <vector>
#include <gtest/gtest.h>
#include "support/bool_xor.hpp"

using namespace AML;

namespace {

// The user boolean type's constructors are ids 3 (false) and 7 (true), so nothing depends on them being 0 and 1.
struct BoolXorTest : public ::testing::Test {
    BoolXorTest();
    ctor_id false_ctor;
    ctor_id true_ctor;
    value falsity;
    value truth;
    bool_xor primitive;
};

BoolXorTest::BoolXorTest()
    : false_ctor(3)
    , true_ctor(7)
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , primitive(false_ctor, true_ctor) {}

}

TEST_F(BoolXorTest, OnlyDifferentInputsGiveTrue) {
    EXPECT_EQ(primitive({&truth, &falsity}), truth);
    EXPECT_EQ(primitive({&falsity, &truth}), truth);
    EXPECT_EQ(primitive({&truth, &truth}), falsity);
    EXPECT_EQ(primitive({&falsity, &falsity}), falsity);
}
