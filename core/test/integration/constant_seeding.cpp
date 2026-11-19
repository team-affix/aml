#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/value_pool.hpp"
#include "value_objects/typed_constant.hpp"

namespace {

using seeder_t = constant_seeder<feature_pool, constant_store>;

// The user offers false and true (type 0) and zero and one (type 1); they are seeded through a real feature pool into
// a real store.
struct ConstantSeedingIntegrationTest : public ::testing::Test {
    ConstantSeedingIntegrationTest();
    value_pool values;
    feature_pool features;
    constant_store constants;
    seeder_t seeder;
    const value* falsity;
    const value* truth;
    const value* zero;
    const value* one;
    std::vector<typed_constant> offered;
};

ConstantSeedingIntegrationTest::ConstantSeedingIntegrationTest()
    : values()
    , features()
    , constants()
    , seeder(features, constants)
    , falsity(values.make(0, {}))
    , truth(values.make(1, {}))
    , zero(values.make(0, {}))
    , one(values.make(1, {zero}))
    , offered{
          typed_constant{.type = 0, .literal = falsity}, typed_constant{.type = 1, .literal = zero},
          typed_constant{.type = 0, .literal = truth}, typed_constant{.type = 1, .literal = one}} {}

}

TEST_F(ConstantSeedingIntegrationTest, EveryOfferedConstantIsStoredInOrderWithItsTypeAndInternedFeature) {
    seeder.seed(offered);
    ASSERT_EQ(constants.constants().size(), 4u);
    for (size_t position = 0; position < offered.size(); ++position) {
        const constant_operand& stored = constants.constants()[position];
        EXPECT_EQ(stored.literal, offered[position].literal);
        EXPECT_EQ(stored.type, offered[position].type);
        EXPECT_EQ(stored.expression, features.make_constant(offered[position].literal));
    }
}

TEST_F(ConstantSeedingIntegrationTest, EachTypeListsItsOwnConstants) {
    seeder.seed(offered);
    EXPECT_EQ(constants.constants_of_type(0), (std::vector<size_t>{0, 2}));
    EXPECT_EQ(constants.constants_of_type(1), (std::vector<size_t>{1, 3}));
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
