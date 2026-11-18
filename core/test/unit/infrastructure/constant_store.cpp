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
    : zero{.ctor = 0, .fields = {}}
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
