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
    EXPECT_EQ(types.decl(zero), (ctor_decl{.type = nat, .tag = 0, .field_types = {}}));
    EXPECT_EQ(types.decl(suc), (ctor_decl{.type = nat, .tag = 1, .field_types = {nat}}));
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
