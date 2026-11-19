#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive.hpp"
#include "infrastructure/razor_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "infrastructure/value_equality.hpp"
#include "infrastructure/value_pool.hpp"

namespace {

struct RazorIntegrationTest : public ::testing::Test {
    RazorIntegrationTest();
    void expect_fit(const razor_runtime& runtime, const bin& training);
    bin xor_training();
    bin is_even_training(size_t count);
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
    value_equality<value_pool> bool_equality;
    razor_options options;
};

RazorIntegrationTest::RazorIntegrationTest()
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
    , bool_equality(
          values, boolean, boolean_encoding{.type = boolean, .false_ctor = false_ctor, .true_ctor = true_ctor})
    , options{.max_applications = 2, .max_constant_size = 1, .exploration_constant = 5.0, .seed = 7} {}

void RazorIntegrationTest::expect_fit(const razor_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<const value*>(training.labels[row]));
    }
}

bin RazorIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = {falsity, truth, truth, falsity}};
}

bin RazorIntegrationTest::is_even_training(size_t count) {
    bin training{.inputs = {typed_column{.type = nat, .cells = {}}}, .labels = {}};
    const value* number = values.make(zero_ctor, {});
    for (size_t index = 0; index < count; ++index) {
        const bool even = (index % 2) == 0;
        training.inputs[0].cells.push_back(number);
        training.labels.push_back(even ? truth : falsity);
        number = values.make(suc_ctor, {number});
    }
    return training;
}

}

TEST_F(RazorIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    razor_runtime runtime(types, values, {&bool_equality}, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model()->content));
    expect_fit(runtime, training);
}

TEST_F(RazorIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    razor_runtime runtime(types, values, {}, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(RazorIntegrationTest, RazorFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, values, {}, training,
                        scout_options{.max_applications = 1, .max_constant_size = 1});
    scout.next();
    razor_runtime runtime(types, values, {}, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(RazorIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    razor_runtime runtime(types, values, {&bool_equality}, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(RazorIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    razor_runtime first(types, values, {&bool_equality}, training, options);
    razor_runtime second(types, values, {&bool_equality}, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(RazorIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    razor_runtime runtime(types, values, {}, xor_training(), options);
    EXPECT_EQ(runtime.best_model(), nullptr);
}

TEST_F(RazorIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    razor_options no_applications = options;
    no_applications.max_applications = 0;
    razor_runtime runtime(types, values, {&bool_equality}, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(RazorIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = {truth, truth, truth, truth};
    razor_runtime runtime(types, values, {&bool_equality}, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}
