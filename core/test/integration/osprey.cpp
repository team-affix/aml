#include <limits>
#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/kite_runtime.hpp"
#include "infrastructure/osprey_manifest.hpp"
#include "infrastructure/osprey_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "support/value_equality.hpp"
#include "value_objects/bin_view.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

struct OspreyIntegrationTest : public ::testing::Test {
    OspreyIntegrationTest();
    void expect_fit(const osprey_runtime& runtime, const bin& training);
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
    osprey_options options;
};

OspreyIntegrationTest::OspreyIntegrationTest()
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
    , options{.max_applications = 2, .exploration_constant = 5.0, .seed = 7, .commit_visits = 100} {}

void OspreyIntegrationTest::expect_fit(const osprey_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<value> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<value>(training.labels.cells[row]));
    }
}

bin OspreyIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin OspreyIntegrationTest::is_even_training(size_t count) {
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

TEST_F(OspreyIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    osprey_runtime runtime(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    expect_fit(runtime, training);
}

TEST_F(OspreyIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    osprey_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(OspreyIntegrationTest, OspreyFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, {}, constants, training,
                        scout_options{.max_applications = 1});
    scout.next();
    osprey_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(OspreyIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    osprey_runtime runtime(types, {bool_equality}, constants, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(OspreyIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    osprey_runtime first(types, {bool_equality}, constants, training, options);
    osprey_runtime second(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(OspreyIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    osprey_runtime runtime(types, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(OspreyIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    osprey_options no_applications = options;
    no_applications.max_applications = 0;
    osprey_runtime runtime(types, {bool_equality}, constants, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(OspreyIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    osprey_runtime runtime(types, {bool_equality}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}

TEST_F(OspreyIntegrationTest, TheSearchCommitsOnceEachTimeTheFrontierGathersItsVisits) {
    const bin training = xor_training();
    osprey_options committing = options;
    committing.commit_visits = 20;
    osprey_manifest manifest(types, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 20; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    EXPECT_EQ(manifest.commitments_.commitment_count(), 0u);
    manifest.mcts_episode_.run(bin_view::of(training));
    EXPECT_EQ(manifest.commitments_.commitment_count(), 1u);
    for (size_t episode = 0; episode < 40; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    EXPECT_EQ(manifest.commitments_.commitment_count(), 3u);
}

TEST_F(OspreyIntegrationTest, TheCommittedRootChoiceIsTakenByEveryLaterEpisode) {
    const bin training = xor_training();
    osprey_options committing = options;
    committing.commit_visits = 20;
    osprey_manifest manifest(types, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 21; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    const mcts_choice committed = manifest.commitments_.find_commitment(0).value();
    const mcts_tree_node_id taken = manifest.tree_walker_.walk(0, committed);
    const mcts_tree_node_id other = manifest.tree_walker_.walk(0, 1 - committed);
    const size_t taken_before = manifest.visits_.get_visits(taken);
    const size_t other_before = manifest.visits_.get_visits(other);
    for (size_t episode = 0; episode < 30; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    EXPECT_EQ(manifest.visits_.get_visits(taken), taken_before + 30);
    EXPECT_EQ(manifest.visits_.get_visits(other), other_before);
}

TEST_F(OspreyIntegrationTest, WithoutCommitmentsOspreySearchesExactlyLikeKite) {
    const bin training = xor_training();
    osprey_options never = options;
    never.commit_visits = std::numeric_limits<size_t>::max();
    osprey_runtime osprey(types, {bool_equality}, constants, training, never);
    kite_runtime kite(types, {bool_equality}, constants, training,
                      kite_options{.max_applications = 2, .exploration_constant = 5.0, .seed = 7});
    for (size_t episode = 0; episode < 100; ++episode) {
        osprey.next();
        kite.next();
        EXPECT_EQ(osprey.best_cost(), kite.best_cost());
    }
}

TEST_F(OspreyIntegrationTest, TheRootIsCommittedToTheBestEpisodesFirstChoice) {
    const bin training = xor_training();
    osprey_options committing = options;
    committing.commit_visits = 20;
    osprey_manifest manifest(types, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 20; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    const mcts_choice first = manifest.best_path_.find_step(0).value();
    manifest.mcts_episode_.run(bin_view::of(training));
    EXPECT_EQ(manifest.commitments_.find_commitment(0), std::optional<mcts_choice>(first));
}

TEST_F(OspreyIntegrationTest, TheBestPathsRewardIsTheBestModelsNegatedCost) {
    const bin training = xor_training();
    osprey_manifest manifest(types, {bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 60; ++episode) {
        manifest.mcts_episode_.run(bin_view::of(training));
        EXPECT_EQ(manifest.best_path_.best_reward(), -static_cast<double>(manifest.best_model_tracker_.best_cost()));
    }
}

TEST_F(OspreyIntegrationTest, EveryCommitmentFollowsTheBestPath) {
    const bin training = xor_training();
    osprey_options committing = options;
    committing.commit_visits = 20;
    osprey_manifest manifest(types, {bool_equality}, committing);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 101; ++episode)
        manifest.mcts_episode_.run(bin_view::of(training));
    ASSERT_GT(manifest.commitments_.commitment_count(), 1u);
    mcts_tree_node_id node = 0;
    while (manifest.commitments_.find_commitment(node).has_value()) {
        EXPECT_EQ(manifest.commitments_.find_commitment(node), manifest.best_path_.find_step(node));
        node = manifest.tree_walker_.walk(node, manifest.commitments_.find_commitment(node).value());
    }
}
