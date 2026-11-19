#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"
#include "support/nat_add.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_primitive.hpp"

namespace {

// The test primitives over a real value pool, stored with their types and reached through the caller and through
// features evaluated on a row. Primitive ids: not 0, and 1, or 2, eq on nat 3, add 4.
struct PrimitivesIntegrationTest : public ::testing::Test {
    PrimitivesIntegrationTest();
    const value* number(size_t count);
    schema types;
    value_pool values;
    type_id boolean;
    size_t false_ctor;
    size_t true_ctor;
    type_id nat;
    size_t zero_ctor;
    size_t suc_ctor;
    const value* falsity;
    const value* truth;
    primitive_type unary_boolean;
    primitive_type binary_boolean;
    primitive_type nat_comparison;
    primitive_type nat_arithmetic;
    primitive_store primitives;
    primitive_caller<primitive_store> caller;
    feature_evaluator<primitive_caller<primitive_store>> evaluator;
};

PrimitivesIntegrationTest::PrimitivesIntegrationTest()
    : types(std::vector<type_def>{
              type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}},
              type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1}}}}})
    , values()
    , boolean(0)
    , false_ctor(0)
    , true_ctor(1)
    , nat(1)
    , zero_ctor(0)
    , suc_ctor(1)
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , unary_boolean{.arg_types = {boolean}, .result_type = boolean}
    , binary_boolean{.arg_types = {boolean, boolean}, .result_type = boolean}
    , nat_comparison{.arg_types = {nat, nat}, .result_type = boolean}
    , nat_arithmetic{.arg_types = {nat, nat}, .result_type = nat}
    , primitives(std::vector<typed_primitive>{
          typed_primitive{.type = unary_boolean, .apply = bool_not<value_pool>(values, false_ctor, true_ctor)},
          typed_primitive{.type = binary_boolean, .apply = bool_and<value_pool>(values, false_ctor, true_ctor)},
          typed_primitive{.type = binary_boolean, .apply = bool_or<value_pool>(values, false_ctor, true_ctor)},
          typed_primitive{.type = nat_comparison, .apply = value_equality<value_pool>(values, false_ctor, true_ctor)},
          typed_primitive{.type = nat_arithmetic, .apply = nat_add<value_pool>(values, suc_ctor)}})
    , caller(primitives)
    , evaluator(caller) {}

const value* PrimitivesIntegrationTest::number(size_t count) {
    const value* result = values.make(zero_ctor, {});
    for (size_t step = 0; step < count; ++step)
        result = values.make(suc_ctor, {result});
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
    for (const value* left : {falsity, truth}) {
        EXPECT_EQ(caller.call(0, {left}), left == truth ? falsity : truth);
        for (const value* right : {falsity, truth}) {
            const bool both = left == truth && right == truth;
            const bool either = left == truth || right == truth;
            EXPECT_EQ(caller.call(1, {left, right}), both ? truth : falsity);
            EXPECT_EQ(caller.call(2, {left, right}), either ? truth : falsity);
        }
    }
}

TEST_F(PrimitivesIntegrationTest, AdditionGivesTheInternedSum) {
    for (size_t left = 0; left < 5; ++left) {
        for (size_t right = 0; right < 5; ++right)
            EXPECT_EQ(caller.call(4, {number(left), number(right)}), number(left + right));
    }
}

TEST_F(PrimitivesIntegrationTest, EqualityHoldsExactlyForEqualNumbers) {
    for (size_t left = 0; left < 4; ++left) {
        for (size_t right = 0; right < 4; ++right)
            EXPECT_EQ(caller.call(3, {number(left), number(right)}), left == right ? truth : falsity);
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
