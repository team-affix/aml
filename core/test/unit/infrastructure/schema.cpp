#include <gtest/gtest.h>
#include "infrastructure/schema.hpp"

namespace {

struct SchemaTest : public ::testing::Test {
    schema types;
};

}

TEST_F(SchemaTest, TypesAreNumberedInDeclarationOrder) {
    EXPECT_EQ(types.declare_type(), 0u);
    EXPECT_EQ(types.declare_type(), 1u);
    EXPECT_EQ(types.type_count(), 2u);
}

TEST_F(SchemaTest, ConstructorsAreNumberedAcrossTypes) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    EXPECT_EQ(types.declare_ctor(boolean, {}), 0u);
    EXPECT_EQ(types.declare_ctor(nat, {}), 1u);
    EXPECT_EQ(types.declare_ctor(boolean, {}), 2u);
}

TEST_F(SchemaTest, AConstructorsTagIsItsPositionWithinItsType) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    types.declare_ctor(boolean, {});
    const ctor_id zero = types.declare_ctor(nat, {});
    const ctor_id suc = types.declare_ctor(nat, {nat});
    EXPECT_EQ(types.decl(zero), (ctor_decl{.type = nat, .tag = 0, .arg_types = {}}));
    EXPECT_EQ(types.decl(suc), (ctor_decl{.type = nat, .tag = 1, .arg_types = {nat}}));
}

TEST_F(SchemaTest, ATypeListsItsConstructorsInTagOrder) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    const ctor_id zero = types.declare_ctor(nat, {});
    const ctor_id falsity = types.declare_ctor(boolean, {});
    const ctor_id suc = types.declare_ctor(nat, {nat});
    EXPECT_EQ(types.ctors(nat), (std::vector<ctor_id>{zero, suc}));
    EXPECT_EQ(types.ctors(boolean), (std::vector<ctor_id>{falsity}));
}

TEST_F(SchemaTest, ANewSchemaHasNoTypes) {
    EXPECT_EQ(types.type_count(), 0u);
}

TEST_F(SchemaTest, ATypeWithoutConstructorsListsNone) {
    const type_id unit = types.declare_type();
    EXPECT_TRUE(types.ctors(unit).empty());
}

TEST_F(SchemaTest, AConstructorKeepsItsArgumentTypesInOrder) {
    const type_id boolean = types.declare_type();
    const type_id nat = types.declare_type();
    const type_id pair = types.declare_type();
    const ctor_id make_pair = types.declare_ctor(pair, {nat, boolean, nat});
    EXPECT_EQ(types.decl(make_pair).arg_types, (std::vector<type_id>{nat, boolean, nat}));
}

TEST_F(SchemaTest, ATypeMayHoldItselfInAArgument) {
    const type_id list = types.declare_type();
    const type_id element = types.declare_type();
    types.declare_ctor(list, {});
    const ctor_id cons = types.declare_ctor(list, {element, list});
    EXPECT_EQ(types.decl(cons), (ctor_decl{.type = list, .tag = 1, .arg_types = {element, list}}));
}
