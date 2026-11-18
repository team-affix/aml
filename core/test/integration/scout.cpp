#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "infrastructure/value_pool.hpp"

namespace {

struct ScoutIntegrationTest : public ::testing::Test {
    ScoutIntegrationTest();
    void expect_fit(const scout_runtime& runtime, const bin& training);
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
    scout_options options;
};

ScoutIntegrationTest::ScoutIntegrationTest()
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
    , options{.max_applications = 1, .max_constant_size = 2} {}

void ScoutIntegrationTest::expect_fit(const scout_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.scope)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<const value*>(training.labels[row]));
    }
}

bin ScoutIntegrationTest::xor_training() {
    return bin{
        .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = {falsity, truth, truth, falsity}};
}

bin ScoutIntegrationTest::is_even_training(size_t count) {
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

TEST_F(ScoutIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    scout_runtime runtime(types, values, {}, xor_training(), options);
    EXPECT_EQ(runtime.best_model(), nullptr);
}

TEST_F(ScoutIntegrationTest, AgreeingLabelsGiveASingleLeaf) {
    bin training = xor_training();
    training.labels = {truth, truth, truth, truth};
    scout_runtime runtime(types, values, {}, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, XorWithoutPrimitivesMatchesOnBothInputs) {
    const bin training = xor_training();
    scout_runtime runtime(types, values, {}, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 10u);
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, IsEvenIsFitByFunnellingThroughPredecessors) {
    const bin training = is_even_training(6);
    scout_runtime runtime(types, values, {}, training, options);
    runtime.next();
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, AnInputUnlikeEveryTrainingRowCanReachAnEmptyBranch) {
    bin training = is_even_training(3);
    training.scope[0].cells.erase(training.scope[0].cells.begin());
    training.labels.erase(training.labels.begin());
    scout_runtime runtime(types, values, {}, training, options);
    runtime.next();
    expect_fit(runtime, training);
    EXPECT_FALSE(runtime.infer({values.make(zero_ctor, {})}).has_value());
}
