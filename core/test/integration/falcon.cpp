#include <limits>
#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/falcon_manifest.hpp"
#include "infrastructure/falcon_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "infrastructure/value_pool.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

namespace {

struct FalconIntegrationTest : public ::testing::Test {
    FalconIntegrationTest();
    void expect_fit(const falcon_runtime& runtime, const bin& training);
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
    typed_primitive bool_equality;
    std::vector<typed_constant> constants;
    falcon_options options;
};

FalconIntegrationTest::FalconIntegrationTest()
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
    , bool_equality{
          .type = primitive_type{.arg_types = {boolean, boolean}, .result_type = boolean},
          .apply = value_equality<value_pool>(values, false_ctor, true_ctor)}
    , constants{
          typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
          typed_constant{.type = nat, .literal = values.make(zero_ctor, {})}}
    , options{.max_applications = 2, .exploration_constant = 5.0, .seed = 7,
               .commit_visits = std::numeric_limits<size_t>::max()} {}

void FalconIntegrationTest::expect_fit(const falcon_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<const value*>(training.labels.cells[row]));
    }
}

bin FalconIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin FalconIntegrationTest::is_even_training(size_t count) {
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

TEST_F(FalconIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    falcon_runtime runtime(types, values, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    expect_fit(runtime, training);
}

TEST_F(FalconIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    falcon_runtime runtime(types, values, {}, constants, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(FalconIntegrationTest, FalconFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, values, {}, constants, training,
                        scout_options{.max_applications = 1});
    scout.next();
    falcon_runtime runtime(types, values, {}, constants, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(FalconIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    falcon_runtime runtime(types, values, {bool_equality}, constants, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(FalconIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    falcon_runtime first(types, values, {bool_equality}, constants, training, options);
    falcon_runtime second(types, values, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(FalconIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    falcon_runtime runtime(types, values, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(FalconIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    falcon_options no_applications = options;
    no_applications.max_applications = 0;
    falcon_runtime runtime(types, values, {bool_equality}, constants, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(FalconIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    falcon_runtime runtime(types, values, {bool_equality}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}

TEST_F(FalconIntegrationTest, TheSearchCommitsOnceEachTimeTheFrontierGathersItsVisits) {
    const bin training = xor_training();
    falcon_options committing = options;
    committing.commit_visits = 20;
    falcon_manifest manifest(types, values, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 20; ++episode)
        manifest.mcts_episode_.run(training);
    EXPECT_EQ(manifest.commitments_.commitment_count(), 0u);
    manifest.mcts_episode_.run(training);
    EXPECT_EQ(manifest.commitments_.commitment_count(), 1u);
    for (size_t episode = 0; episode < 40; ++episode)
        manifest.mcts_episode_.run(training);
    EXPECT_EQ(manifest.commitments_.commitment_count(), 3u);
}

TEST_F(FalconIntegrationTest, TheCommittedRootChoiceIsTakenByEveryLaterEpisode) {
    const bin training = xor_training();
    falcon_options committing = options;
    committing.commit_visits = 20;
    falcon_manifest manifest(types, values, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 21; ++episode)
        manifest.mcts_episode_.run(training);
    const mcts_choice committed = manifest.commitments_.find_commitment(0).value();
    const mcts_tree_node_id taken = manifest.tree_walker_.walk(0, committed);
    const mcts_tree_node_id other = manifest.tree_walker_.walk(0, 1 - committed);
    const size_t taken_before = manifest.visits_.get_visits(taken);
    const size_t other_before = manifest.visits_.get_visits(other);
    for (size_t episode = 0; episode < 30; ++episode)
        manifest.mcts_episode_.run(training);
    EXPECT_EQ(manifest.visits_.get_visits(taken), taken_before + 30);
    EXPECT_EQ(manifest.visits_.get_visits(other), other_before);
}

TEST_F(FalconIntegrationTest, APrimitiveIsChosenInOneChoiceAmongAllPrimitives) {
    const bin training = xor_training();
    falcon_options never = options;
    never.commit_visits = std::numeric_limits<size_t>::max();
    falcon_manifest manifest(types, values, {bool_equality, bool_equality, bool_equality}, never);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 50; ++episode)
        manifest.mcts_episode_.run(training);
    // The root chooses between stopping (0) and continuing (1) synthesis; continuing, one choice picks the primitive.
    const mcts_tree_node_id continued = manifest.tree_walker_.walk(0, 1);
    for (mcts_choice primitive = 0; primitive < 3; ++primitive)
        EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, primitive)), 0u);
}

TEST_F(FalconIntegrationTest, TheRootIsCommittedToTheBestEpisodesFirstChoice) {
    const bin training = xor_training();
    falcon_options committing = options;
    committing.commit_visits = 20;
    falcon_manifest manifest(types, values, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 20; ++episode)
        manifest.mcts_episode_.run(training);
    const mcts_choice first = manifest.best_path_.find_step(0).value();
    manifest.mcts_episode_.run(training);
    EXPECT_EQ(manifest.commitments_.find_commitment(0), std::optional<mcts_choice>(first));
}

TEST_F(FalconIntegrationTest, TheBestPathsRewardIsTheBestModelsNegatedCost) {
    const bin training = xor_training();
    falcon_manifest manifest(types, values, {bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 60; ++episode) {
        manifest.mcts_episode_.run(training);
        EXPECT_EQ(manifest.best_path_.best_reward(), -static_cast<double>(manifest.best_model_tracker_.best_cost()));
    }
}

TEST_F(FalconIntegrationTest, EveryCommitmentFollowsTheBestPath) {
    const bin training = xor_training();
    falcon_options committing = options;
    committing.commit_visits = 20;
    falcon_manifest manifest(types, values, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 101; ++episode)
        manifest.mcts_episode_.run(training);
    ASSERT_GT(manifest.commitments_.commitment_count(), 1u);
    mcts_tree_node_id node = 0;
    while (manifest.commitments_.find_commitment(node).has_value()) {
        EXPECT_EQ(manifest.commitments_.find_commitment(node), manifest.best_path_.find_step(node));
        node = manifest.tree_walker_.walk(node, manifest.commitments_.find_commitment(node).value());
    }
}

TEST_F(FalconIntegrationTest, NextReturnsTheEpisodesModelAndItsCost) {
    const bin training = xor_training();
    falcon_runtime runtime(types, values, {bool_equality}, constants, training, options);
    const episode_outcome first = runtime.next();
    EXPECT_EQ(runtime.best_model(), std::optional<model_node>(first.model));
    EXPECT_EQ(runtime.best_cost(), first.cost);
    for (size_t episode = 0; episode < 50; ++episode) {
        const episode_outcome outcome = runtime.next();
        EXPECT_LE(runtime.best_cost(), outcome.cost);
    }
}
