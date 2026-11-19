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
    : zero{.ctor = 0, .fields = {}}
    , one{.ctor = 1, .fields = {&zero}}
    , hasher() {}

}

TEST_F(ValueHashTest, EqualValuesHashEqual) {
    EXPECT_EQ(hasher(value{.ctor = 1, .fields = {&zero}}), hasher(one));
}

TEST_F(ValueHashTest, ADifferentConstructorChangesTheHash) {
    EXPECT_NE(hasher(value{.ctor = 2, .fields = {&zero}}), hasher(one));
}

TEST_F(ValueHashTest, ADifferentFieldChangesTheHash) {
    EXPECT_NE(hasher(value{.ctor = 1, .fields = {&one}}), hasher(one));
}

TEST_F(ValueHashTest, FieldOrderChangesTheHash) {
    EXPECT_NE(hasher(value{.ctor = 2, .fields = {&zero, &one}}), hasher(value{.ctor = 2, .fields = {&one, &zero}}));
}

TEST_F(ValueHashTest, AnExtraFieldChangesTheHash) {
    EXPECT_NE(hasher(value{.ctor = 2, .fields = {&zero}}), hasher(value{.ctor = 2, .fields = {&zero, &zero}}));
}
