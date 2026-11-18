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
    , bool_equality(values, boolean, boolean_encoding{.type = boolean, .false_ctor = false_ctor, .true_ctor = true_ctor})
    , options{.max_applications = 2, .max_constant_size = 1, .exploration_constant = 5.0, .seed = 7} {}

void RazorIntegrationTest::expect_fit(const razor_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.scope)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<const value*>(training.labels[row]));
    }
}

bin RazorIntegrationTest::xor_training() {
    return bin{
        .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = {falsity, truth, truth, falsity}};
}

bin RazorIntegrationTest::is_even_training(size_t count) {
    bin training{.scope = {typed_column{.type = nat, .cells = {}}}, .labels = {}};
    const value* number = values.make(zero_ctor, {});
    for (size_t index = 0; index < count; ++index) {
        const bool even = (index % 2) == 0;
        training.scope[0].cells.push_back(number);
        training.labels.push_back(even ? truth : falsity);
        number = values.make(suc_ctor, {number});
    }
    return training;
}

}

TEST_F(RazorIntegrationTest, XorIsFoundAsAMatchOnInputEquality) {
    const bin training = xor_training();
    razor_runtime runtime(types, values, {&bool_equality}, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    EXPECT_EQ(runtime.best_cost(), 6u);
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
