#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_chooser.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/column_applier.hpp"
#include "infrastructure/constant_candidate_finder.hpp"
#include "infrastructure/constant_enumerator.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_dispatcher.hpp"
#include "infrastructure/primitive_option_finder.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/synthesis_starter.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_equality.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/witness_detector.hpp"

using ::testing::Invoke;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace {

// Stands in for MCTS: every choice of the session goes through it.
struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (size_t));
};

size_t last_option(size_t option_count) {
    return option_count - 1;
}

using chooser_t = NiceMock<MockChooseOption>;
using feature_sizer_t = feature_sizer<value_sizer>;
using primitive_dispatcher_t = primitive_dispatcher<primitive>;
using column_applier_t = column_applier<primitive_dispatcher_t>;
using variable_candidate_finder_t = variable_candidate_finder<feature_pool, feature_sizer_t, uniform_value_finder>;
using constant_enumerator_t = constant_enumerator<schema, schema, value_pool>;
using constant_seeder_t = constant_seeder<schema, constant_enumerator_t, feature_pool, constant_store>;
using constant_candidate_finder_t = constant_candidate_finder<constant_store, feature_sizer_t>;
using app_candidate_finder_t =
    app_candidate_finder<column_applier_t, uniform_value_finder, feature_pool, feature_sizer_t>;
using witness_detector_t = witness_detector<cheapest_feature_index, cheapest_feature_index, application_log>;
using primitive_option_finder_t =
    primitive_option_finder<primitive_dispatcher_t, primitive_dispatcher_t, witness_detector_t>;
using argument_option_finder_t =
    argument_option_finder<cheapest_feature_index, cheapest_feature_index, witness_detector_t>;
using argument_chooser_t = argument_chooser<argument_option_finder_t, chooser_t>;
using synthesis_starter_t = synthesis_starter<
    cheapest_feature_index, cheapest_feature_index, application_log, constant_candidate_finder_t,
    cheapest_feature_index, variable_candidate_finder_t, cheapest_feature_index>;
using feature_synthesizer_t = feature_synthesizer<
    synthesis_starter_t, application_log, primitive_option_finder_t, chooser_t, primitive_dispatcher_t,
    argument_chooser_t, application_log, cheapest_feature_index, cheapest_feature_index, app_candidate_finder_t,
    cheapest_feature_index>;

// XOR over two boolean inputs, with boolean equality as the only primitive and false and true as the constants. The
// operands are false (0), true (1), x0 (2) and x1 (3) until something is derived.
struct FeatureSynthesisIntegrationTest : public ::testing::Test {
    FeatureSynthesisIntegrationTest();
    feature_synthesizer_t make(size_t max_applications);
    schema types;
    value_pool values;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    const value* falsity;
    const value* truth;
    value_equality<value_pool> equality;
    bin training;
    feature_pool features;
    constant_store constants;
    cheapest_feature_index constant_candidates;
    cheapest_feature_index feature_candidates;
    application_log applications;
    uniform_value_finder uniform_values;
    value_sizer value_sizes;
    feature_sizer_t feature_sizes;
    primitive_dispatcher_t primitives;
    column_applier_t column_applier;
    variable_candidate_finder_t variable_candidate_finder;
    constant_enumerator_t constant_enumerator;
    constant_seeder_t constant_seeder;
    constant_candidate_finder_t constant_candidate_finder;
    app_candidate_finder_t app_candidate_finder;
    witness_detector_t witness_detector;
    primitive_option_finder_t primitive_option_finder;
    argument_option_finder_t argument_option_finder;
    chooser_t chooser;
    argument_chooser_t argument_chooser;
    synthesis_starter_t synthesis_starter;
};

FeatureSynthesisIntegrationTest::FeatureSynthesisIntegrationTest()
    : types()
    , values()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , equality(values, boolean, boolean_encoding{.type = boolean, .false_ctor = false_ctor, .true_ctor = true_ctor})
    , training{
          .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                    typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
          .labels = {falsity, truth, truth, falsity}}
    , features()
    , constants()
    , constant_candidates()
    , feature_candidates()
    , applications()
    , uniform_values()
    , value_sizes()
    , feature_sizes(value_sizes)
    , primitives(std::vector<primitive>{&equality})
    , column_applier(primitives)
    , variable_candidate_finder(features, feature_sizes, uniform_values)
    , constant_enumerator(types, types, values)
    , constant_seeder(types, constant_enumerator, features, constants)
    , constant_candidate_finder(constants, feature_sizes)
    , app_candidate_finder(column_applier, uniform_values, features, feature_sizes)
    , witness_detector(constant_candidates, feature_candidates, applications)
    , primitive_option_finder(primitives, primitives, witness_detector)
    , argument_option_finder(constant_candidates, feature_candidates, witness_detector)
    , chooser()
    , argument_chooser(argument_option_finder, chooser)
    , synthesis_starter(
          constant_candidates, feature_candidates, applications, constant_candidate_finder, constant_candidates,
          variable_candidate_finder, feature_candidates) {
    constant_seeder.seed(1);
}

feature_synthesizer_t FeatureSynthesisIntegrationTest::make(size_t max_applications) {
    return feature_synthesizer_t{
        synthesis_starter, applications, primitive_option_finder, chooser, primitives, argument_chooser,
        applications, constant_candidates, feature_candidates, app_candidate_finder, feature_candidates,
        max_applications};
}

}

TEST_F(FeatureSynthesisIntegrationTest, StoppingAtOnceLeavesTheInputs) {
    feature_synthesizer_t synthesizer = make(10);
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    const std::vector<candidate> candidates = synthesizer.synthesize(training);
    ASSERT_EQ(candidates.size(), 2u);
    EXPECT_EQ(candidates[0].expression, features.make_var(0));
    EXPECT_EQ(candidates[1].expression, features.make_var(1));
}

TEST_F(FeatureSynthesisIntegrationTest, ChoosingEqualityOfTheInputsDerivesItAtCostThree) {
    feature_synthesizer_t synthesizer = make(10);
    InSequence order;
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(2));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(3));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    const std::vector<candidate> candidates = synthesizer.synthesize(training);
    ASSERT_EQ(candidates.size(), 3u);
    EXPECT_EQ(candidates[2].expression, features.make_app(0, {features.make_var(0), features.make_var(1)}));
    EXPECT_EQ(candidates[2].cost, 3u);
    EXPECT_EQ(candidates[2].observed.cells, (column{truth, falsity, falsity, truth}));
    EXPECT_EQ(applications.count(), 1u);
}

TEST_F(FeatureSynthesisIntegrationTest, ATupleOfConstantsIsNeverOffered) {
    feature_synthesizer_t synthesizer = make(10);
    InSequence order;
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(0));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    synthesizer.synthesize(training);
}

TEST_F(FeatureSynthesisIntegrationTest, ApplyingUntilNoWitnessIsLeftTriesEveryUsefulTupleOnce) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    const std::vector<candidate> candidates = synthesizer.synthesize(training);
    const size_t operand_count = constant_candidates.candidates().size() + candidates.size();
    const size_t constant_count = constant_candidates.candidates().size();
    EXPECT_EQ(applications.count(), operand_count * operand_count - constant_count * constant_count);
    EXPECT_GT(candidates.size(), 2u);
}

TEST_F(FeatureSynthesisIntegrationTest, TheSessionAppliesNoMoreThanTheCap) {
    feature_synthesizer_t synthesizer = make(5);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    synthesizer.synthesize(training);
    EXPECT_EQ(applications.count(), 5u);
}

TEST_F(FeatureSynthesisIntegrationTest, EveryDerivedFeatureIsHeterogeneousAndHasItsOwnColumn) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    std::set<column> columns;
    for (const candidate& entry : synthesizer.synthesize(training)) {
        EXPECT_FALSE(uniform_values.find(entry.observed.cells).has_value());
        EXPECT_TRUE(columns.insert(entry.observed.cells).second);
    }
}

TEST_F(FeatureSynthesisIntegrationTest, ANewSessionForgetsTheLastOne) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    synthesizer.synthesize(training);
    ON_CALL(chooser, choose(_)).WillByDefault(Return(0));
    EXPECT_EQ(synthesizer.synthesize(training).size(), 2u);
    EXPECT_EQ(applications.count(), 0u);
}
