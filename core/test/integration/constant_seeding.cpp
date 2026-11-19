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

using enumerator_t = constant_enumerator<schema, value_pool>;
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
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    type_id nat;
    ctor_ref zero_ctor;
    ctor_ref suc_ctor;
    type_id bool_pair;
    ctor_ref pair_ctor;
    type_id empty_type;
    enumerator_t enumerator;
    seeder_t seeder;
};

ConstantSeedingIntegrationTest::ConstantSeedingIntegrationTest()
    : types(std::vector<type_def>{
              type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}},
              type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1}}}},
              type_def{.ctors = {ctor_def{.arg_types = {0, 0}}}},
              type_def{.ctors = {}}})
    , values()
    , features()
    , constants()
    , sizes()
    , boolean(0)
    , false_ctor{.type = boolean, .tag = 0}
    , true_ctor{.type = boolean, .tag = 1}
    , nat(1)
    , zero_ctor{.type = nat, .tag = 0}
    , suc_ctor{.type = nat, .tag = 1}
    , bool_pair(2)
    , pair_ctor{.type = bool_pair, .tag = 0}
    , empty_type(3)
    , enumerator(types, values)
    , seeder(types, enumerator, features, constants) {}

}

TEST_F(ConstantSeedingIntegrationTest, EveryValueUpToTheSizeIsSeededOnceAsAnInternedConstant) {
    seeder.seed(3);
    std::set<const value*> seen;
    for (const constant_operand& constant : constants.constants()) {
        EXPECT_LE(sizes.size(constant.literal), 3u);
        EXPECT_EQ(constant.literal->ctor.type, constant.type);
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
