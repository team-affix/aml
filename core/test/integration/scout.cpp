#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "infrastructure/value_pool.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

namespace {

struct ScoutIntegrationTest : public ::testing::Test {
    ScoutIntegrationTest();
    void expect_fit(const scout_runtime& runtime, const bin& training);
    bin xor_training();
    bin is_even_training(size_t count);
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
    std::vector<typed_constant> constants;
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
    , constants{
          typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
          typed_constant{.type = nat, .literal = values.make(zero_ctor, {})}}
    , options{.max_applications = 1} {}

void ScoutIntegrationTest::expect_fit(const scout_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<const value*>(training.labels.cells[row]));
    }
}

bin ScoutIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin ScoutIntegrationTest::is_even_training(size_t count) {
    bin training{.inputs = {typed_column{.type = nat, .cells = {}}},
                 .labels = typed_column{.type = boolean, .cells = {}}};
    const value* number = values.make(zero_ctor, {});
    for (size_t index = 0; index < count; ++index) {
        const bool even = (index % 2) == 0;
        training.inputs[0].cells.push_back(number);
        training.labels.cells.push_back(even ? truth : falsity);
        number = values.make(suc_ctor, {number});
    }
    return training;
}

}

TEST_F(ScoutIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    scout_runtime runtime(types, values, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(ScoutIntegrationTest, AgreeingLabelsGiveASingleLeaf) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, XorWithoutPrimitivesMatchesOnBothInputsWhereTheyDifferAndOutputsX1WhereX0IsFalse) {
    const bin training = xor_training();
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    // match x0 (2) + [x0 false: leaf x1 (1)] + [x0 true: match x1 (2) + two constant leaves (2)]
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, IsEvenIsFitByFunnellingThroughPredecessors) {
    const bin training = is_even_training(6);
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, AnInputUnlikeEveryTrainingRowCanReachAnEmptyBranch) {
    bin training = is_even_training(3);
    training.inputs[0].cells.erase(training.inputs[0].cells.begin());
    training.labels.cells.erase(training.labels.cells.begin());
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    expect_fit(runtime, training);
    EXPECT_FALSE(runtime.infer({values.make(zero_ctor, {})}).has_value());
}

TEST_F(ScoutIntegrationTest, MoreEpisodesChangeNothing) {
    const bin training = is_even_training(5);
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    const model_node model = runtime.best_model().value();
    const size_t cost = runtime.best_cost();
    runtime.next();
    runtime.next();
    EXPECT_EQ(runtime.best_model(), std::optional<model_node>(model));
    EXPECT_EQ(runtime.best_cost(), cost);
}

TEST_F(ScoutIntegrationTest, ScoutStopsSynthesisAtOnceSoPrimitivesDoNotChangeItsModel) {
    const bin training = xor_training();
    const typed_primitive equality{
        .type = primitive_type{.arg_types = {boolean, boolean}, .result_type = boolean},
        .apply = value_equality<value_pool>(values, false_ctor, true_ctor)};
    scout_runtime with_primitive(types, values, {equality}, constants, training, options);
    scout_runtime without_primitive(types, values, {}, constants, training, options);
    with_primitive.next();
    without_primitive.next();
    EXPECT_EQ(with_primitive.best_cost(), without_primitive.best_cost());
}

TEST_F(ScoutIntegrationTest, IsEvenFitsLargerTrainingSets) {
    const bin training = is_even_training(12);
    scout_runtime runtime(types, values, {}, constants, training, options);
    runtime.next();
    expect_fit(runtime, training);
}

TEST_F(ScoutIntegrationTest, NextReturnsTheEpisodesModelAndItsCost) {
    scout_runtime runtime(types, values, {}, constants, xor_training(), options);
    const episode_outcome outcome = runtime.next();
    EXPECT_EQ(runtime.best_model(), std::optional<model_node>(outcome.model));
    EXPECT_EQ(runtime.best_cost(), outcome.cost);
}
