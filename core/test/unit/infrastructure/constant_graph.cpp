#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/constant_graph.hpp"

using namespace AML;

namespace {

struct ConstantGraphTest : public ::testing::Test {
    ConstantGraphTest();
    constant_record atom(size_t index, std::type_index type);
    constant_graph constants;
};

ConstantGraphTest::ConstantGraphTest()
    : constants() {}

constant_record ConstantGraphTest::atom(size_t index, std::type_index type) {
    return constant_record{.origin = constant_derivation{constant_derivation::atom{.index = index}}, .type = type};
}

}

TEST_F(ConstantGraphTest, InsertedConstantsAreNumberedInOrder) {
    EXPECT_EQ(constants.insert(atom(0, typeid(int))), 0u);
    EXPECT_EQ(constants.insert(atom(1, typeid(bool))), 1u);
}

TEST_F(ConstantGraphTest, AConstantsRecordIsKept) {
    constants.insert(atom(0, typeid(int)));
    EXPECT_EQ(constants.record(0), atom(0, typeid(int)));
}

TEST_F(ConstantGraphTest, EachTypeListsItsConstantsInOrder) {
    constants.insert(atom(0, typeid(int)));
    constants.insert(atom(1, typeid(bool)));
    constants.insert(atom(2, typeid(int)));
    EXPECT_EQ(constants.of_type(typeid(int)), (std::vector<constant_id>{0, 2}));
    EXPECT_EQ(constants.of_type(typeid(bool)), (std::vector<constant_id>{1}));
}

TEST_F(ConstantGraphTest, ATypeWithNoConstantsListsNone) {
    constants.insert(atom(0, typeid(int)));
    EXPECT_TRUE(constants.of_type(typeid(double)).empty());
}

TEST_F(ConstantGraphTest, AnApplicationIsAConstantLikeAnAtom) {
    constants.insert(atom(0, typeid(int)));
    const constant_id product = constants.insert(constant_record{
        .origin = constant_derivation{constant_derivation::application{.prim = 1, .args = {0, 0}}},
        .type = typeid(int)});
    EXPECT_EQ(product, 1u);
    EXPECT_EQ(constants.of_type(typeid(int)), (std::vector<constant_id>{0, 1}));
}
