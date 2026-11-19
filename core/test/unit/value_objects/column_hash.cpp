#include <gtest/gtest.h>
#include "value_objects/column_hash.hpp"

namespace {

struct ColumnHashTest : public ::testing::Test {
    ColumnHashTest();
    value falsity;
    value truth;
    column_hash hasher;
};

ColumnHashTest::ColumnHashTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , hasher() {}

}

TEST_F(ColumnHashTest, EqualColumnsHashEqual) {
    EXPECT_EQ(hasher(column{&falsity, &truth}), hasher(column{&falsity, &truth}));
}

TEST_F(ColumnHashTest, CellOrderChangesTheHash) {
    EXPECT_NE(hasher(column{&falsity, &truth}), hasher(column{&truth, &falsity}));
}

TEST_F(ColumnHashTest, LengthChangesTheHash) {
    EXPECT_NE(hasher(column{&falsity}), hasher(column{&falsity, &falsity}));
}

TEST_F(ColumnHashTest, EmptyColumnsHashEqual) {
    EXPECT_EQ(hasher(column{}), hasher(column{}));
}

TEST_F(ColumnHashTest, DifferentValuesHashApart) {
    EXPECT_NE(hasher(column{&truth}), hasher(column{&falsity}));
}
