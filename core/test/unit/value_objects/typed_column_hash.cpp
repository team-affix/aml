#include <gtest/gtest.h>
#include "value_objects/typed_column_hash.hpp"

using namespace AML;

namespace {

struct TypedColumnHashTest : public ::testing::Test {
    TypedColumnHashTest();
    value falsity;
    value truth;
    typed_column_hash hasher;
};

TypedColumnHashTest::TypedColumnHashTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , hasher() {}

}

TEST_F(TypedColumnHashTest, EqualTypedColumnsHashEqual) {
    EXPECT_EQ(hasher(typed_column{.type = 0, .cells = {falsity, truth}}),
              hasher(typed_column{.type = 0, .cells = {falsity, truth}}));
}

TEST_F(TypedColumnHashTest, CellOrderChangesTheHash) {
    EXPECT_NE(hasher(typed_column{.type = 0, .cells = {falsity, truth}}),
              hasher(typed_column{.type = 0, .cells = {truth, falsity}}));
}

TEST_F(TypedColumnHashTest, LengthChangesTheHash) {
    EXPECT_NE(hasher(typed_column{.type = 0, .cells = {falsity}}),
              hasher(typed_column{.type = 0, .cells = {falsity, falsity}}));
}

TEST_F(TypedColumnHashTest, TheTypeChangesTheHash) {
    EXPECT_NE(hasher(typed_column{.type = 0, .cells = {falsity, truth}}),
              hasher(typed_column{.type = 1, .cells = {falsity, truth}}));
}

TEST_F(TypedColumnHashTest, EmptyColumnsOfOneTypeHashEqual) {
    EXPECT_EQ(hasher(typed_column{.type = 2, .cells = {}}), hasher(typed_column{.type = 2, .cells = {}}));
}

TEST_F(TypedColumnHashTest, DifferentValuesHashApart) {
    EXPECT_NE(hasher(typed_column{.type = 0, .cells = {truth}}), hasher(typed_column{.type = 0, .cells = {falsity}}));
}

TEST_F(TypedColumnHashTest, CellsHashByStructure) {
    const value pair{.ctor = 4, .args = {truth, falsity}};
    const value same_pair{.ctor = 4, .args = {value{.ctor = 1, .args = {}}, value{.ctor = 0, .args = {}}}};
    EXPECT_EQ(hasher(typed_column{.type = 3, .cells = {pair}}), hasher(typed_column{.type = 3, .cells = {same_pair}}));
}
