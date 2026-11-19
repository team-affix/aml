#include <gtest/gtest.h>
#include "infrastructure/constant_store.hpp"

namespace {

struct ConstantStoreTest : public ::testing::Test {
    ConstantStoreTest();
    value zero;
    feature zero_feature;
    constant_store constants;
};

ConstantStoreTest::ConstantStoreTest()
    : zero{.tag = 0, .args = {}}
    , zero_feature{feature::constant{.literal = &zero}}
    , constants() {}

}

TEST_F(ConstantStoreTest, StartsEmpty) {
    EXPECT_TRUE(constants.constants().empty());
}

TEST_F(ConstantStoreTest, InsertedConstantsAreKeptInOrder) {
    const constant_operand first{.expression = &zero_feature, .literal = &zero, .type = 0};
    const constant_operand second{.expression = &zero_feature, .literal = &zero, .type = 1};
    constants.insert(first);
    constants.insert(second);
    EXPECT_EQ(constants.constants(), (std::vector<constant_operand>{first, second}));
}

TEST_F(ConstantStoreTest, EachTypeListsItsConstantsPositionsInOrder) {
    constants.insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 1});
    constants.insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 0});
    constants.insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 1});
    EXPECT_EQ(constants.constants_of_type(0), (std::vector<size_t>{1}));
    EXPECT_EQ(constants.constants_of_type(1), (std::vector<size_t>{0, 2}));
}

TEST_F(ConstantStoreTest, ATypeWithNoConstantsListsNone) {
    constants.insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 0});
    EXPECT_TRUE(constants.constants_of_type(4).empty());
}
