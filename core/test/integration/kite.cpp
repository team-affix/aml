#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/kite_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

struct KiteIntegrationTest : public ::testing::Test {
    KiteIntegrationTest();
    void expect_fit(const kite_runtime& runtime, const bin& training);
    bin xor_training();
    bin is_even_training(size_t count);
    schema types;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    type_id nat;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    value falsity;
    value truth;
    typed_primitive bool_equality;
    std::vector<typed_constant> constants;
    kite_options options;
};

KiteIntegrationTest::KiteIntegrationTest()
    : types()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , falsity(value{.ctor = false_ctor, .args = {}})
    , truth(value{.ctor = true_ctor, .args = {}})
    , bool_equality{
          .type = primitive_type{.arg_types = {boolean, boolean}, .result_type = boolean},
          .apply = value_equality(false_ctor, true_ctor)}
    , constants{
          typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
          typed_constant{.type = nat, .literal = value{.ctor = zero_ctor, .args = {}}}}
    , options{.max_applications = 2, .exploration_constant = 5.0, .seed = 7} {}

void KiteIntegrationTest::expect_fit(const kite_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<value> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<value>(training.labels.cells[row]));
    }
}

bin KiteIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin KiteIntegrationTest::is_even_training(size_t count) {
    bin training{.inputs = {typed_column{.type = nat, .cells = {}}},
                 .labels = typed_column{.type = boolean, .cells = {}}};
    value number = value{.ctor = zero_ctor, .args = {}};
    for (size_t index = 0; index < count; ++index) {
        const bool even = (index % 2) == 0;
        training.inputs[0].cells.push_back(number);
        training.labels.cells.push_back(even ? truth : falsity);
        number = value{.ctor = suc_ctor, .args = {number}};
    }
    return training;
}

}

TEST_F(KiteIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    kite_runtime runtime(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    expect_fit(runtime, training);
}

TEST_F(KiteIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    kite_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(KiteIntegrationTest, KiteFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, {}, constants, training,
                        scout_options{.max_applications = 1});
    scout.next();
    kite_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(KiteIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    kite_runtime runtime(types, {bool_equality}, constants, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(KiteIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    kite_runtime first(types, {bool_equality}, constants, training, options);
    kite_runtime second(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(KiteIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    kite_runtime runtime(types, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(KiteIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    kite_options no_applications = options;
    no_applications.max_applications = 0;
    kite_runtime runtime(types, {bool_equality}, constants, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(KiteIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    kite_runtime runtime(types, {bool_equality}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}
