#include <set>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/constant_enumerator.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"

namespace {

using enumerator_t = constant_enumerator<schema, schema, value_pool>;
using seeder_t = constant_seeder<schema, enumerator_t, feature_pool, constant_store>;

// Booleans, naturals, pairs of booleans and a type without constructors.
struct ConstantSeedingIntegrationTest : public ::testing::Test {
    ConstantSeedingIntegrationTest();
    schema types;
    value_pool values;
    feature_pool features;
    constant_store constants;
    value_sizer sizes;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    type_id nat;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    type_id bool_pair;
    ctor_id pair_ctor;
    type_id empty_type;
    enumerator_t enumerator;
    seeder_t seeder;
};

ConstantSeedingIntegrationTest::ConstantSeedingIntegrationTest()
    : types()
    , values()
    , features()
    , constants()
    , sizes()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , bool_pair(types.declare_type())
    , pair_ctor(types.declare_ctor(bool_pair, {boolean, boolean}))
    , empty_type(types.declare_type())
    , enumerator(types, types, values)
    , seeder(types, enumerator, features, constants) {}

}

TEST_F(ConstantSeedingIntegrationTest, EveryValueUpToTheSizeIsSeededOnceAsAnInternedConstant) {
    seeder.seed(3);
    std::set<const value*> seen;
    for (const constant_operand& constant : constants.constants()) {
        EXPECT_LE(sizes.size(constant.literal), 3u);
        EXPECT_EQ(types.decl(constant.literal->ctor).type, constant.type);
        EXPECT_EQ(constant.expression, features.make_constant(constant.literal));
        EXPECT_TRUE(seen.insert(constant.literal).second);
    }
    // 2 booleans + zero, one, two + 4 boolean pairs
    EXPECT_EQ(constants.constants().size(), 9u);
}

TEST_F(ConstantSeedingIntegrationTest, EachTypeListsItsOwnConstantsSmallestFirst) {
    seeder.seed(3);
    const std::vector<size_t>& naturals = constants.constants_of_type(nat);
    ASSERT_EQ(naturals.size(), 3u);
    EXPECT_EQ(constants.constants()[naturals[0]].literal, values.make(zero_ctor, {}));
    EXPECT_EQ(constants.constants()[naturals[1]].literal, values.make(suc_ctor, {values.make(zero_ctor, {})}));
    for (size_t position = 1; position < naturals.size(); ++position) {
        EXPECT_LE(sizes.size(constants.constants()[naturals[position - 1]].literal),
                  sizes.size(constants.constants()[naturals[position]].literal));
    }
}

TEST_F(ConstantSeedingIntegrationTest, ATypeWithoutConstructorsSeedsNothing) {
    seeder.seed(4);
    EXPECT_TRUE(constants.constants_of_type(empty_type).empty());
}

TEST_F(ConstantSeedingIntegrationTest, ASizeOfOneSeedsOnlyNullaryConstructors) {
    seeder.seed(1);
    EXPECT_EQ(constants.constants_of_type(boolean).size(), 2u);
    EXPECT_EQ(constants.constants_of_type(nat).size(), 1u);
    EXPECT_TRUE(constants.constants_of_type(bool_pair).empty());
}
