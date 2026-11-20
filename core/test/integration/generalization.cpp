#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/razor_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "infrastructure/value_pool.hpp"
#include "support/nat_add.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

// Booleans, naturals and pairs of a natural and a boolean. Models whose leaves output variables or synthesized
// features must infer correctly on inputs no training row held.
struct GeneralizationIntegrationTest : public ::testing::Test {
    GeneralizationIntegrationTest();
    const value* number(size_t count);
    const value* pair_of(const value* first, const value* second);
    bin unary_training(
        type_id input_type, const column& inputs, type_id label_type, const column& labels);
    schema types;
    value_pool values;
    type_id boolean;
    size_t false_ctor;
    size_t true_ctor;
    type_id nat;
    size_t zero_ctor;
    size_t suc_ctor;
    type_id pair;
    size_t pair_ctor;
    const value* falsity;
    const value* truth;
    typed_primitive addition;
    std::vector<typed_constant> constants;
    scout_options scout_settings;
    razor_options razor_settings;
};

GeneralizationIntegrationTest::GeneralizationIntegrationTest()
    : types()
    , values()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , pair(types.declare_type())
    , pair_ctor(types.declare_ctor(pair, {nat, boolean}))
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , addition{
          .type = primitive_type{.arg_types = {nat, nat}, .result_type = nat},
          .apply = nat_add<value_pool>(values, suc_ctor)}
    , constants{
          typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
          typed_constant{.type = nat, .literal = values.make(zero_ctor, {})}}
    , scout_settings{.max_applications = 2}
    , razor_settings{.max_applications = 1, .exploration_constant = 3.0, .seed = 11} {}

const value* GeneralizationIntegrationTest::number(size_t count) {
    const value* result = values.make(zero_ctor, {});
    for (size_t step = 0; step < count; ++step)
        result = values.make(suc_ctor, {result});
    return result;
}

const value* GeneralizationIntegrationTest::pair_of(const value* first, const value* second) {
    return values.make(pair_ctor, {first, second});
}

bin GeneralizationIntegrationTest::unary_training(
    type_id input_type, const column& inputs, type_id label_type, const column& labels) {
    return bin{.inputs = {typed_column{.type = input_type, .cells = inputs}},
               .labels = typed_column{.type = label_type, .cells = labels}};
}

}

TEST_F(GeneralizationIntegrationTest, IdentityIsOneVariableLeafAndHoldsForUnseenInputs) {
    const bin training = unary_training(nat, {number(0), number(1), number(2)}, nat, {number(0), number(1), number(2)});
    scout_runtime runtime(types, values, {}, constants, training, scout_settings);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
    EXPECT_EQ(runtime.infer({number(25)}), std::optional<const value*>(number(25)));
}

TEST_F(GeneralizationIntegrationTest, PredecessorOutputsTheBoundArgumentAndHoldsForUnseenInputs) {
    const bin training = unary_training(
        nat, {number(0), number(1), number(2), number(3)}, nat, {number(0), number(0), number(1), number(2)});
    scout_runtime runtime(types, values, {}, constants, training, scout_settings);
    runtime.next();
    // match x0 (2) + [zero: leaf x0 (1)] + [suc: leaf x1 (1)]
    EXPECT_EQ(runtime.best_cost(), 4u);
    EXPECT_EQ(runtime.infer({number(0)}), std::optional<const value*>(number(0)));
    EXPECT_EQ(runtime.infer({number(17)}), std::optional<const value*>(number(16)));
}

TEST_F(GeneralizationIntegrationTest, AProjectionFunnelsIntoTheArgumentThatIsTheOutput) {
    const bin training = unary_training(
        pair, {pair_of(number(0), truth), pair_of(number(2), falsity), pair_of(number(1), truth)},
        nat, {number(0), number(2), number(1)});
    scout_runtime runtime(types, values, {}, constants, training, scout_settings);
    runtime.next();
    // match x0 (2) + [pair: leaf x1 (1)]
    EXPECT_EQ(runtime.best_cost(), 3u);
    EXPECT_EQ(runtime.infer({pair_of(number(9), falsity)}), std::optional<const value*>(number(9)));
}

TEST_F(GeneralizationIntegrationTest, TheOtherProjectionPicksTheOtherArgument) {
    const bin training = unary_training(
        pair, {pair_of(number(0), truth), pair_of(number(2), falsity), pair_of(number(1), truth)},
        boolean, {truth, falsity, truth});
    scout_runtime runtime(types, values, {}, constants, training, scout_settings);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 3u);
    EXPECT_EQ(runtime.infer({pair_of(number(40), falsity)}), std::optional<const value*>(falsity));
}

TEST_F(GeneralizationIntegrationTest, ASecondInputIsChosenOverAConstantWhenItIsTheOutput) {
    const bin training{
        .inputs = {typed_column{.type = nat, .cells = {number(0), number(1), number(2)}},
                  typed_column{.type = nat, .cells = {number(3), number(3), number(3)}}},
        .labels = typed_column{.type = nat, .cells = {number(3), number(3), number(3)}}};
    scout_runtime runtime(types, values, {}, constants, training, scout_settings);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
    EXPECT_EQ(runtime.infer({number(5), number(8)}), std::optional<const value*>(number(8)));
}

TEST_F(GeneralizationIntegrationTest, RazorDoublesByAddingTheInputToItself) {
    const bin training = unary_training(
        nat, {number(0), number(1), number(2), number(3)}, nat, {number(0), number(2), number(4), number(6)});
    razor_runtime runtime(types, values, {addition}, constants, training, razor_settings);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf add(x0, x0): 1 + 1 + 1
    EXPECT_EQ(runtime.best_cost(), 3u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    EXPECT_EQ(runtime.infer({number(7)}), std::optional<const value*>(number(14)));
}

TEST_F(GeneralizationIntegrationTest, RazorAddsTwoInputs) {
    const bin training{
        .inputs = {typed_column{.type = nat, .cells = {number(0), number(1), number(2), number(1)}},
                  typed_column{.type = nat, .cells = {number(2), number(0), number(1), number(3)}}},
        .labels = typed_column{.type = nat, .cells = {number(2), number(1), number(3), number(4)}}};
    razor_runtime runtime(types, values, {addition}, constants, training, razor_settings);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    EXPECT_EQ(runtime.best_cost(), 3u);
    EXPECT_EQ(runtime.infer({number(6), number(5)}), std::optional<const value*>(number(11)));
}
