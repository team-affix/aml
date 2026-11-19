#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/schema.hpp"

namespace {

// Types: 0 = bool {false, true}, 1 = nat {zero, suc(nat)}, 2 = list of nat {nil, cons(nat, list)}.
struct SchemaTest : public ::testing::Test {
    SchemaTest();
    type_def boolean;
    type_def nat;
    type_def list;
    schema types;
};

SchemaTest::SchemaTest()
    : boolean{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}}
    , nat{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1}}}}
    , list{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1, 2}}}}
    , types(std::vector<type_def>{boolean, nat, list}) {}

}

TEST_F(SchemaTest, CountsItsTypes) {
    EXPECT_EQ(types.type_count(), 3u);
}

TEST_F(SchemaTest, ATypesIdIsItsPosition) {
    EXPECT_EQ(types.def(0), boolean);
    EXPECT_EQ(types.def(1), nat);
    EXPECT_EQ(types.def(2), list);
}

TEST_F(SchemaTest, ConstructorsKeepTheirTagOrderAndArgumentTypes) {
    EXPECT_EQ(types.def(2).ctors.size(), 2u);
    EXPECT_TRUE(types.def(2).ctors[0].arg_types.empty());
    EXPECT_EQ(types.def(2).ctors[1].arg_types, (std::vector<type_id>{1, 2}));
}

TEST_F(SchemaTest, ATypeMayReferToItselfAndToLaterTypes) {
    const schema forward(std::vector<type_def>{
        type_def{.ctors = {ctor_def{.arg_types = {1}}, ctor_def{.arg_types = {0}}}},
        type_def{.ctors = {ctor_def{.arg_types = {}}}}});
    EXPECT_EQ(forward.def(0).ctors[0].arg_types, (std::vector<type_id>{1}));
}

TEST_F(SchemaTest, ASchemaWithoutTypesIsEmpty) {
    const schema empty(std::vector<type_def>{});
    EXPECT_EQ(empty.type_count(), 0u);
}

TEST_F(SchemaTest, ATypeWithoutConstructorsIsAllowed) {
    const schema uninhabited(std::vector<type_def>{type_def{.ctors = {}}});
    EXPECT_TRUE(uninhabited.def(0).ctors.empty());
}

TEST_F(SchemaTest, TheTypesAreHeldNotReferredTo) {
    std::vector<type_def> given{boolean};
    const schema held(given);
    given[0].ctors.clear();
    EXPECT_EQ(held.def(0), boolean);
}
