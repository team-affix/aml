#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/schema.hpp"

namespace {

struct SchemaTest : public ::testing::Test {
    schema types;
};

}

TEST_F(SchemaTest, ASchemaStartsWithoutTypes) {
    EXPECT_EQ(types.type_count(), 0u);
}

TEST_F(SchemaTest, EachDeclaredTypeGetsTheNextId) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    EXPECT_EQ(boolean, 0u);
    EXPECT_EQ(nat, 1u);
    EXPECT_EQ(types.type_count(), 2u);
}

TEST_F(SchemaTest, ADeclaredTypeStartsWithoutConstructors) {
    const type_id boolean = types.declare_type();
    EXPECT_TRUE(types.def(boolean).ctors.empty());
}

TEST_F(SchemaTest, EachConstructorGetsTheNextTagOfItsType) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    EXPECT_EQ(types.declare_ctor(boolean, {}), 0u);
    EXPECT_EQ(types.declare_ctor(nat, {}), 0u);
    EXPECT_EQ(types.declare_ctor(boolean, {}), 1u);
    EXPECT_EQ(types.def(boolean).ctors.size(), 2u);
    EXPECT_EQ(types.def(nat).ctors.size(), 1u);
}

TEST_F(SchemaTest, ConstructorsKeepTheirArgumentTypes) {
    const type_id nat = types.declare_type();
    const type_id list = types.declare_type();
    types.declare_ctor(list, {});
    const size_t cons = types.declare_ctor(list, {nat, list});
    EXPECT_EQ(types.def(list).ctors[cons].arg_types, (std::vector<type_id>{nat, list}));
}

TEST_F(SchemaTest, ATypeMayReferToItself) {
    const type_id nat = types.declare_type();
    types.declare_ctor(nat, {});
    const size_t suc = types.declare_ctor(nat, {nat});
    EXPECT_EQ(types.def(nat).ctors[suc].arg_types, (std::vector<type_id>{nat}));
}

TEST_F(SchemaTest, TypesMayReferToEachOther) {
    const type_id tree = types.declare_type();
    const type_id forest = types.declare_type();
    const size_t node = types.declare_ctor(tree, {forest});
    const size_t cons = types.declare_ctor(forest, {tree, forest});
    EXPECT_EQ(types.def(tree).ctors[node].arg_types, (std::vector<type_id>{forest}));
    EXPECT_EQ(types.def(forest).ctors[cons].arg_types, (std::vector<type_id>{tree, forest}));
}

TEST_F(SchemaTest, AConstructorOfAnUndeclaredTypeIsRejected) {
    EXPECT_THROW(types.declare_ctor(0, {}), std::logic_error);
}

TEST_F(SchemaTest, AnArgumentOfAnUndeclaredTypeIsRejected) {
    const type_id boolean = types.declare_type();
    EXPECT_THROW(types.declare_ctor(boolean, {1}), std::logic_error);
}
