#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/bool_and.hpp"
#include "infrastructure/bool_not.hpp"
#include "infrastructure/bool_or.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/nat_add.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_dispatcher.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_equality.hpp"
#include "infrastructure/value_pool.hpp"

namespace {

// Every native primitive over a real value pool, reached through the dispatcher and through features evaluated on
// a row. Primitive ids: not 0, and 1, or 2, eq on nat 3, add 4.
struct PrimitivesIntegrationTest : public ::testing::Test {
    PrimitivesIntegrationTest();
    const value* number(size_t count);
    schema types;
    value_pool values;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    type_id nat;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    const value* falsity;
    const value* truth;
    boolean_encoding booleans;
    nat_encoding nats;
    bool_not<value_pool> negation;
    bool_and<value_pool> conjunction;
    bool_or<value_pool> disjunction;
    value_equality<value_pool> nat_equality;
    nat_add<value_pool> addition;
    primitive_dispatcher<primitive> primitives;
    feature_pool features;
    feature_evaluator<primitive_dispatcher<primitive>> evaluator;
};

PrimitivesIntegrationTest::PrimitivesIntegrationTest()
    : types()
    , values()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , booleans{.type = boolean, .false_ctor = false_ctor, .true_ctor = true_ctor}
    , nats{.type = nat, .zero_ctor = zero_ctor, .suc_ctor = suc_ctor}
    , negation(values, booleans)
    , conjunction(values, booleans)
    , disjunction(values, booleans)
    , nat_equality(values, nat, booleans)
    , addition(values, nats)
    , primitives(std::vector<primitive>{&negation, &conjunction, &disjunction, &nat_equality, &addition})
    , features()
    , evaluator(primitives) {}

const value* PrimitivesIntegrationTest::number(size_t count) {
    const value* result = values.make(zero_ctor, {});
    for (size_t step = 0; step < count; ++step)
        result = values.make(suc_ctor, {result});
    return result;
}

}

TEST_F(PrimitivesIntegrationTest, TheDispatcherReportsEachPrimitivesSignature) {
    EXPECT_EQ(primitives.count(), 5u);
    EXPECT_EQ(primitives.signature(0), (primitive_signature{.arg_types = {boolean}, .result_type = boolean}));
    EXPECT_EQ(primitives.signature(3), (primitive_signature{.arg_types = {nat, nat}, .result_type = boolean}));
    EXPECT_EQ(primitives.signature(4), (primitive_signature{.arg_types = {nat, nat}, .result_type = nat}));
}

TEST_F(PrimitivesIntegrationTest, BooleanPrimitivesFollowTheirTruthTables) {
    for (const value* left : {falsity, truth}) {
        EXPECT_EQ(primitives.apply(0, {left}), left == truth ? falsity : truth);
        for (const value* right : {falsity, truth}) {
            const bool both = left == truth && right == truth;
            const bool either = left == truth || right == truth;
            EXPECT_EQ(primitives.apply(1, {left, right}), both ? truth : falsity);
            EXPECT_EQ(primitives.apply(2, {left, right}), either ? truth : falsity);
        }
    }
}

TEST_F(PrimitivesIntegrationTest, AdditionGivesTheInternedSum) {
    for (size_t left = 0; left < 5; ++left) {
        for (size_t right = 0; right < 5; ++right)
            EXPECT_EQ(primitives.apply(4, {number(left), number(right)}), number(left + right));
    }
}

TEST_F(PrimitivesIntegrationTest, EqualityHoldsExactlyForEqualNumbers) {
    for (size_t left = 0; left < 4; ++left) {
        for (size_t right = 0; right < 4; ++right)
            EXPECT_EQ(primitives.apply(3, {number(left), number(right)}), left == right ? truth : falsity);
    }
}

TEST_F(PrimitivesIntegrationTest, NestedFeaturesEvaluateThroughTheDispatcher) {
    const feature* sum = features.make_app(4, {features.make_var(0), features.make_constant(number(2))});
    const feature* is_five = features.make_app(3, {sum, features.make_constant(number(5))});
    const feature* not_five = features.make_app(0, {is_five});
    EXPECT_EQ(evaluator.evaluate(sum, {number(3)}), number(5));
    EXPECT_EQ(evaluator.evaluate(is_five, {number(3)}), truth);
    EXPECT_EQ(evaluator.evaluate(not_five, {number(3)}), falsity);
    EXPECT_EQ(evaluator.evaluate(not_five, {number(4)}), truth);
}
