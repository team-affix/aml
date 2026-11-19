#include <gtest/gtest.h>
#include "value_objects/value_hash.hpp"

namespace {

struct ValueHashTest : public ::testing::Test {
    ValueHashTest();
    value zero;
    value one;
    value_hash hasher;
};

ValueHashTest::ValueHashTest()
    : zero{.tag = 0, .args = {}}
    , one{.tag = 1, .args = {&zero}}
    , hasher() {}

}

TEST_F(ValueHashTest, EqualValuesHashEqual) {
    EXPECT_EQ(hasher(value{.tag = 1, .args = {&zero}}), hasher(one));
}

TEST_F(ValueHashTest, ADifferentConstructorChangesTheHash) {
    EXPECT_NE(hasher(value{.tag = 2, .args = {&zero}}), hasher(one));
}

TEST_F(ValueHashTest, ADifferentArgumentChangesTheHash) {
    EXPECT_NE(hasher(value{.tag = 1, .args = {&one}}), hasher(one));
}

TEST_F(ValueHashTest, ArgumentOrderChangesTheHash) {
    EXPECT_NE(hasher(value{.tag = 2, .args = {&zero, &one}}),
              hasher(value{.tag = 2, .args = {&one, &zero}}));
}

TEST_F(ValueHashTest, AnExtraArgumentChangesTheHash) {
    EXPECT_NE(hasher(value{.tag = 2, .args = {&zero}}),
              hasher(value{.tag = 2, .args = {&zero, &zero}}));
}
