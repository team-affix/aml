#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/schema.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"
#include "support/nat_add.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

// The test primitives, stored with their types and reached through the caller and through
// features evaluated on a row. Primitive ids: not 0, and 1, or 2, eq on nat 3, add 4.
struct PrimitivesIntegrationTest : public ::testing::Test {
    PrimitivesIntegrationTest();
    value number(size_t count);
    schema types;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    type_id nat;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    value falsity;
    value truth;
    primitive_type unary_boolean;
    primitive_type binary_boolean;
    primitive_type nat_comparison;
    primitive_type nat_arithmetic;
    primitive_store primitives;
    primitive_caller<primitive_store> caller;
    feature_evaluator<primitive_caller<primitive_store>> evaluator;
};

PrimitivesIntegrationTest::PrimitivesIntegrationTest()
    : types()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , falsity(value{.ctor = false_ctor, .args = {}})
    , truth(value{.ctor = true_ctor, .args = {}})
    , unary_boolean{.arg_types = {boolean}, .result_type = boolean}
    , binary_boolean{.arg_types = {boolean, boolean}, .result_type = boolean}
    , nat_comparison{.arg_types = {nat, nat}, .result_type = boolean}
    , nat_arithmetic{.arg_types = {nat, nat}, .result_type = nat}
    , primitives(std::vector<typed_primitive>{
          typed_primitive{.type = unary_boolean, .apply = bool_not(false_ctor, true_ctor)},
          typed_primitive{.type = binary_boolean, .apply = bool_and(false_ctor, true_ctor)},
          typed_primitive{.type = binary_boolean, .apply = bool_or(false_ctor, true_ctor)},
          typed_primitive{.type = nat_comparison, .apply = value_equality(false_ctor, true_ctor)},
          typed_primitive{.type = nat_arithmetic, .apply = nat_add(suc_ctor)}})
    , caller(primitives)
    , evaluator(caller) {}

value PrimitivesIntegrationTest::number(size_t count) {
    value result = value{.ctor = zero_ctor, .args = {}};
    for (size_t step = 0; step < count; ++step)
        result = value{.ctor = suc_ctor, .args = {result}};
    return result;
}

}

TEST_F(PrimitivesIntegrationTest, TheStoreKeepsEachPrimitivesType) {
    EXPECT_EQ(primitives.count(), 5u);
    EXPECT_EQ(primitives.type(0), unary_boolean);
    EXPECT_EQ(primitives.type(3), nat_comparison);
    EXPECT_EQ(primitives.type(4), nat_arithmetic);
}

TEST_F(PrimitivesIntegrationTest, BooleanPrimitivesFollowTheirTruthTables) {
    for (const value* left : {&falsity, &truth}) {
        EXPECT_EQ(caller.call(0, {left}), *left == truth ? falsity : truth);
        for (const value* right : {&falsity, &truth}) {
            const bool both = *left == truth && *right == truth;
            const bool either = *left == truth || *right == truth;
            EXPECT_EQ(caller.call(1, {left, right}), both ? truth : falsity);
            EXPECT_EQ(caller.call(2, {left, right}), either ? truth : falsity);
        }
    }
}

TEST_F(PrimitivesIntegrationTest, AdditionGivesTheSum) {
    for (size_t left = 0; left < 5; ++left) {
        for (size_t right = 0; right < 5; ++right) {
            const value left_number = number(left);
            const value right_number = number(right);
            EXPECT_EQ(caller.call(4, {&left_number, &right_number}), number(left + right));
        }
    }
}

TEST_F(PrimitivesIntegrationTest, EqualityHoldsExactlyForEqualNumbers) {
    for (size_t left = 0; left < 4; ++left) {
        for (size_t right = 0; right < 4; ++right) {
            const value left_number = number(left);
            const value right_number = number(right);
            EXPECT_EQ(caller.call(3, {&left_number, &right_number}), left == right ? truth : falsity);
        }
    }
}

TEST_F(PrimitivesIntegrationTest, NestedFeaturesEvaluateThroughTheCaller) {
    const feature input{feature::var{.index = 0}};
    const feature two{feature::constant{.literal = number(2)}};
    const feature five{feature::constant{.literal = number(5)}};
    const feature sum{feature::app{.prim = 4, .args = {input, two}}};
    const feature is_five{feature::app{.prim = 3, .args = {sum, five}}};
    const feature not_five{feature::app{.prim = 0, .args = {is_five}}};
    EXPECT_EQ(evaluator.evaluate(sum, {number(3)}), number(5));
    EXPECT_EQ(evaluator.evaluate(is_five, {number(3)}), truth);
    EXPECT_EQ(evaluator.evaluate(not_five, {number(3)}), falsity);
    EXPECT_EQ(evaluator.evaluate(not_five, {number(4)}), truth);
}
