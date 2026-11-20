#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/heron_manifest.hpp"
#include "infrastructure/heron_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_runtime.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

struct HeronIntegrationTest : public ::testing::Test {
    HeronIntegrationTest();
    void expect_fit(const heron_runtime& runtime, const bin& training);
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
    heron_options options;
};

HeronIntegrationTest::HeronIntegrationTest()
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
    , options{.max_applications = 2, .exploration_constant = 4.0, .focus_width = 0.1, .focus_rate = 0.1,
               .focus_bonus = 0.25, .seed = 7} {}

void HeronIntegrationTest::expect_fit(const heron_runtime& runtime, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<value> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(runtime.infer(inputs), std::optional<value>(training.labels.cells[row]));
    }
}

bin HeronIntegrationTest::xor_training() {
    return bin{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}};
}

bin HeronIntegrationTest::is_even_training(size_t count) {
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

TEST_F(HeronIntegrationTest, XorIsFoundAsOneSynthesizedOutput) {
    const bin training = xor_training();
    heron_runtime runtime(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 200; ++episode)
        runtime.next();
    // leaf eq(eq(x0, x1), false): 1 + (1 + 1 + 1) + 1
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_TRUE(std::holds_alternative<model_node::leaf>(runtime.best_model().value().content));
    expect_fit(runtime, training);
}

TEST_F(HeronIntegrationTest, EveryEpisodesModelFitsTheTrainingData) {
    const bin training = is_even_training(6);
    heron_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 20; ++episode) {
        runtime.next();
        expect_fit(runtime, training);
    }
}

TEST_F(HeronIntegrationTest, HeronFindsAModelNoCostlierThanScouts) {
    const bin training = is_even_training(6);
    scout_runtime scout(types, {}, constants, training,
                        scout_options{.max_applications = 1});
    scout.next();
    heron_runtime runtime(types, {}, constants, training, options);
    for (size_t episode = 0; episode < 300; ++episode)
        runtime.next();
    EXPECT_LE(runtime.best_cost(), scout.best_cost());
}

TEST_F(HeronIntegrationTest, TheBestCostNeverRises) {
    const bin training = xor_training();
    heron_runtime runtime(types, {bool_equality}, constants, training, options);
    size_t previous = runtime.best_cost();
    for (size_t episode = 0; episode < 100; ++episode) {
        runtime.next();
        EXPECT_LE(runtime.best_cost(), previous);
        previous = runtime.best_cost();
    }
}

TEST_F(HeronIntegrationTest, TheSameSeedFindsTheSameModel) {
    const bin training = xor_training();
    heron_runtime first(types, {bool_equality}, constants, training, options);
    heron_runtime second(types, {bool_equality}, constants, training, options);
    for (size_t episode = 0; episode < 50; ++episode) {
        first.next();
        second.next();
        EXPECT_EQ(first.best_cost(), second.best_cost());
    }
}

TEST_F(HeronIntegrationTest, NoModelExistsBeforeTheFirstEpisode) {
    heron_runtime runtime(types, {}, constants, xor_training(), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(HeronIntegrationTest, ACapOfZeroNeverAppliesAPrimitive) {
    const bin training = xor_training();
    heron_options no_applications = options;
    no_applications.max_applications = 0;
    heron_runtime runtime(types, {bool_equality}, constants, training, no_applications);
    for (size_t episode = 0; episode < 100; ++episode)
        runtime.next();
    // Without synthesis XOR needs a match on both inputs: the best is scout's cost 7.
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_fit(runtime, training);
}

TEST_F(HeronIntegrationTest, AgreeingLabelsNeedNoSearch) {
    bin training = xor_training();
    training.labels = typed_column{.type = boolean, .cells = {truth, truth, truth, truth}};
    heron_runtime runtime(types, {bool_equality}, constants, training, options);
    runtime.next();
    EXPECT_EQ(runtime.best_cost(), 1u);
}

TEST_F(HeronIntegrationTest, APrimitiveIsChosenInOneChoiceAmongAllPrimitives) {
    const bin training = xor_training();
    heron_manifest manifest(types, {bool_equality, bool_equality, bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 50; ++episode)
        manifest.mcts_episode_.run(training);
    // The root chooses between stopping (0) and continuing (1) synthesis; continuing, one choice picks the primitive.
    const mcts_tree_node_id continued = manifest.tree_walker_.walk(0, 1);
    for (mcts_choice primitive = 0; primitive < 3; ++primitive)
        EXPECT_GT(manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, primitive)), 0u);
}

TEST_F(HeronIntegrationTest, NextReturnsTheEpisodesModelAndItsCost) {
    const bin training = xor_training();
    heron_runtime runtime(types, {bool_equality}, constants, training, options);
    const episode_outcome first = runtime.next();
    EXPECT_EQ(runtime.best_model(), std::optional<model_node>(first.model));
    EXPECT_EQ(runtime.best_cost(), first.cost);
    for (size_t episode = 0; episode < 50; ++episode) {
        const episode_outcome outcome = runtime.next();
        EXPECT_LE(runtime.best_cost(), outcome.cost);
    }
}

TEST_F(HeronIntegrationTest, InterchangeablePrimitivesConcentrateOnTheFirst) {
    const bin training = xor_training();
    heron_manifest manifest(types, {bool_equality, bool_equality, bool_equality}, options);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 2000; ++episode)
        manifest.mcts_episode_.run(training);
    // The three primitives are the same, so their children's means are near ties; the first gets the focus bonus and
    // more than 1.25 times the visits of each other.
    const mcts_tree_node_id continued = manifest.tree_walker_.walk(0, 1);
    const size_t first = manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, 0));
    for (mcts_choice primitive = 1; primitive < 3; ++primitive)
        EXPECT_GT(4 * first, 5 * manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, primitive)));
}

TEST_F(HeronIntegrationTest, WithoutABonusInterchangeablePrimitivesShareTheVisits) {
    const bin training = xor_training();
    heron_options no_bonus = options;
    no_bonus.focus_bonus = 0.0;
    heron_manifest manifest(types, {bool_equality, bool_equality, bool_equality}, no_bonus);
    manifest.constant_seeder_.seed(constants);
    for (size_t episode = 0; episode < 2000; ++episode)
        manifest.mcts_episode_.run(training);
    const mcts_tree_node_id continued = manifest.tree_walker_.walk(0, 1);
    const size_t first = manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, 0));
    for (mcts_choice primitive = 1; primitive < 3; ++primitive)
        EXPECT_LT(4 * first, 5 * manifest.visits_.get_visits(manifest.tree_walker_.walk(continued, primitive)));
}
