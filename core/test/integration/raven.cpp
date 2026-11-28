#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/raven_manifest.hpp"
#include "infrastructure/raven_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "support/value_equality.hpp"
#include "value_objects/bin_view.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

struct RavenIntegrationTest : public ::testing::Test {
    RavenIntegrationTest();
    void expect_fit(const raven_runtime& runtime, const bin& training);
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
    raven_options options;
};

RavenIntegrationTest::RavenIntegrationTest()
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
    , options{.max_applications = 2, .exploration_constant = 4.0, .best_weight = 1.0, .fresh_weight = 0.75,
               .made_weight = 0.5, .seed = 7} {}

void RavenIntegrationTest::expect_fit(const raven_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<value> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<value>(training.labels.cells[row]));
    }
}

bin RavenIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin RavenIntegrationTest::is_even_training(size_t count) {
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

TEST_F(RavenIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    raven_runtime runtime(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    expect_fit(runtime, training);
}

TEST_F(RavenIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    raven_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(RavenIntegrationTest, RavenFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, {}, constants, training,
                        scout_options{.max_applications = 1});
    scout.next();
    raven_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(RavenIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    raven_runtime runtime(types, {bool_equality}, constants, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(RavenIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    raven_runtime first(types, {bool_equality}, constants, training, options);
    raven_runtime second(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(RavenIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    raven_runtime runtime(types, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(RavenIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    raven_options no_applications = options;
    no_applications.max_applications = 0;
    raven_runtime runtime(types, {bool_equality}, constants, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(RavenIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    raven_runtime runtime(types, {bool_equality}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}

TEST_F(RavenIntegrationTest, APrimitiveIsChosenThroughTwoWayChoices) {
    const bin training = xor_training();
    raven_manifest manifest(types, {bool_equality, bool_equality, bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 50; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    // The root chooses between stopping (0) and continuing (1) synthesis. Continuing, the three primitives are halved:
    // the lower half (the first two) or the upper (the third), then one of the lower two. No node has a third child.
    const mcts_tree_node_id continued = manifest.tree_walker_.walk(0, 1);
    const mcts_tree_node_id lower = manifest.tree_walker_.walk(continued, 0);
    EXPECT_GT(manifest.visits_.get_visits(lower), 0u);
    EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, 1)), 0u);
    EXPECT_EQ(manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, 2)), 0u);
    EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(lower, 0)), 0u);
    EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(lower, 1)), 0u);
}

TEST_F(RavenIntegrationTest, NextReturnsTheEpisodesModelAndItsCost) {
    const bin training = xor_training();
    raven_runtime runtime(types, {bool_equality}, constants, training, options);
    const episode_outcome first = runtime.next();
    EXPECT_EQ(runtime.best_model(), std::optional<model_node>(first.model));
    EXPECT_EQ(runtime.best_cost(), first.cost);
    for (size_t episode = 0; episode < 50; ++episode) {
        const episode_outcome outcome = runtime.next();
        EXPECT_LE(runtime.best_cost(), outcome.cost);
    }
}

TEST_F(RavenIntegrationTest, OperandsOfOneQuadrantAreChosenThroughTwoWayChoices) {
    const bin training = xor_training();
    raven_manifest manifest(types, {bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 200; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    // Continuing (1), the one primitive is taken without a choice. The first argument's operands (the constants false
    // and true, then the variables x0 and x1) are all given and fresh, one quadrant, so there is no quadrant choice;
    // the four are halved twice, so the argument's node has two children and each of those has two.
    const mcts_tree_node_id argument = manifest.tree_walker_.walk(0, 1);
    for (mcts_choice half = 0; half < 2; ++half) {
        const mcts_tree_node_id halved = manifest.tree_walker_.walk(argument, half);
        EXPECT_GT(manifest.visits_.get_visits(halved), 0u);
        EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(halved, 0)), 0u);
        EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(halved, 1)), 0u);
    }
    for (mcts_choice beyond = 2; beyond < 4; ++beyond)
        EXPECT_EQ(manifest.visits_.get_visits(manifest.tree_walker_.walk(argument, beyond)), 0u);
}

TEST_F(RavenIntegrationTest, TwoInputsOfOneQuadrantAreMatchedOnInOneTwoWayChoice) {
    const bin training = xor_training();
    raven_options no_synthesis = options;
    no_synthesis.max_applications = 0;
    raven_manifest manifest(types, {bool_equality}, no_synthesis);
    manifest.constant_seeder_.seed(constants);
    const bin_view viewed = bin_view::of(training);
    for (size_t episode = 0; episode < 50; ++episode)
        manifest.mcts_episode_.run(viewed);
    // With no applications the session offers the two inputs; halving two options is one two-way choice.
    for (mcts_choice feature = 0; feature < 2; ++feature)
        EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(0, feature)), 0u);
}

TEST_F(RavenIntegrationTest, TheRootsBestRewardIsTheBestModelsNegatedCost) {
    const bin training = xor_training();
    raven_manifest manifest(types, {bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    const bin_view viewed = bin_view::of(training);
    for (size_t episode = 0; episode < 100; ++episode) {
        manifest.mcts_episode_.run(viewed);
        EXPECT_EQ(manifest.best_rewards_.get_value(0), -static_cast<double>(manifest.best_model_tracker_.best_cost()));
    }
}

TEST_F(RavenIntegrationTest, TheBestWeightChangesTheSearch) {
    const bin training = xor_training();
    raven_options no_best = options;
    no_best.best_weight = 0.0;
    raven_runtime blended(types, {bool_equality}, constants, training, options);
    raven_runtime plain(types, {bool_equality}, constants, training, no_best);
    bool differed = false;
    for (size_t episode = 0; episode < 200; ++episode)
        differed = differed || blended.next().cost != plain.next().cost;
    EXPECT_TRUE(differed);
}

TEST_F(RavenIntegrationTest, EveryEpisodesModelInfersItsTrainingLabelsThroughInheritedVariables) {
    const bin training = xor_training();
    raven_manifest manifest(types, {bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    const bin_view viewed = bin_view::of(training);
    bool matched_on_an_application = false;
    for (size_t episode = 0; episode < 300; ++episode) {
        const episode_outcome outcome = manifest.mcts_episode_.run(viewed);
        const model_node::match* root = std::get_if<model_node::match>(&outcome.model.content);
        const bool by_application = root != nullptr && std::holds_alternative<feature::app>(root->scrutinee.content);
        matched_on_an_application = matched_on_an_application || by_application;
        for (size_t row = 0; row < training.labels.cells.size(); ++row) {
            std::vector<value> inputs;
            for (const typed_column& input : training.inputs)
                inputs.push_back(input.cells[row]);
            EXPECT_EQ(manifest.model_evaluator_.evaluate(outcome.model, inputs),
                      std::optional<value>(training.labels.cells[row]));
        }
    }
    EXPECT_TRUE(matched_on_an_application);
}
