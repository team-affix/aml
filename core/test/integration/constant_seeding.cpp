#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "value_objects/typed_constant.hpp"

namespace {

using seeder_t = constant_seeder<constant_store>;

// The user offers false and true (booleans) and zero and one (naturals); they are seeded into a real store.
struct ConstantSeedingIntegrationTest : public ::testing::Test {
    ConstantSeedingIntegrationTest();
    schema types;
    type_id boolean;
    size_t false_ctor;
    size_t true_ctor;
    type_id nat;
    size_t zero_ctor;
    size_t suc_ctor;
    value_pool values;
    constant_store constants;
    seeder_t seeder;
    const value* falsity;
    const value* truth;
    const value* zero;
    const value* one;
    std::vector<typed_constant> offered;
};

ConstantSeedingIntegrationTest::ConstantSeedingIntegrationTest()
    : types()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , values()
    , constants()
    , seeder(constants)
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , zero(values.make(zero_ctor, {}))
    , one(values.make(suc_ctor, {zero}))
    , offered{
          typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = nat, .literal = zero},
          typed_constant{.type = boolean, .literal = truth}, typed_constant{.type = nat, .literal = one}} {}

}

TEST_F(ConstantSeedingIntegrationTest, EveryOfferedConstantIsStoredInOrderWithItsTypeAndConstantFeature) {
    seeder.seed(offered);
    ASSERT_EQ(constants.constants().size(), 4u);
    for (size_t position = 0; position < offered.size(); ++position) {
        const constant_operand& stored = constants.constants()[position];
        EXPECT_EQ(stored.literal, offered[position].literal);
        EXPECT_EQ(stored.type, offered[position].type);
        EXPECT_EQ(stored.expression, feature{feature::constant{.literal = offered[position].literal}});
    }
}

TEST_F(ConstantSeedingIntegrationTest, EachTypeListsItsOwnConstants) {
    seeder.seed(offered);
    EXPECT_EQ(constants.constants_of_type(boolean), (std::vector<size_t>{0, 2}));
    EXPECT_EQ(constants.constants_of_type(nat), (std::vector<size_t>{1, 3}));
}

TEST_F(ConstantSeedingIntegrationTest, NoOfferedConstantsSeedNothing) {
    seeder.seed({});
    EXPECT_TRUE(constants.constants().empty());
}

TEST_F(ConstantSeedingIntegrationTest, ValuesWithTheSameTagButDifferentTypesStayApartByType) {
    seeder.seed(offered);
    EXPECT_EQ(falsity, zero);
    EXPECT_EQ(constants.constants()[0].type, 0u);
    EXPECT_EQ(constants.constants()[1].type, 1u);
}
