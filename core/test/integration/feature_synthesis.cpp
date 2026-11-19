#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_chooser.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/constant_enumerator.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/operand_applier.hpp"
#include "infrastructure/operand_table.hpp"
#include "infrastructure/output_finder.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_signature_reader.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/primitive_applier.hpp"
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
using primitive_store_t = primitive_store<primitive>;
using signature_reader_t = primitive_signature_reader<primitive_store_t>;
using primitive_caller_t = primitive_caller<primitive_store_t>;
using variable_candidate_finder_t = variable_candidate_finder<feature_pool, feature_sizer_t, uniform_value_finder>;
using constant_enumerator_t = constant_enumerator<schema, schema, value_pool>;
using constant_seeder_t = constant_seeder<schema, constant_enumerator_t, feature_pool, constant_store>;
using operand_table_t =
    operand_table<constant_store, constant_store, cheapest_feature_index, cheapest_feature_index>;
using operand_applier_t = operand_applier<operand_table_t, primitive_caller_t>;
using app_candidate_finder_t =
    app_candidate_finder<operand_applier_t, uniform_value_finder, operand_table_t, feature_pool, feature_sizer_t>;
using witness_detector_t = witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log>;
using primitive_option_finder_t =
    primitive_option_finder<primitive_store_t, signature_reader_t, witness_detector_t>;
using argument_option_finder_t =
    argument_option_finder<operand_table_t, operand_table_t, witness_detector_t>;
using argument_chooser_t = argument_chooser<argument_option_finder_t, chooser_t>;
using primitive_applier_t = primitive_applier<
    signature_reader_t, argument_chooser_t, application_log, app_candidate_finder_t, cheapest_feature_index>;
using synthesis_starter_t =
    synthesis_starter<cheapest_feature_index, application_log, variable_candidate_finder_t, cheapest_feature_index>;
using feature_synthesizer_t = feature_synthesizer<
    synthesis_starter_t, application_log, primitive_option_finder_t, chooser_t, primitive_applier_t,
    cheapest_feature_index, output_finder>;

// XOR over two boolean inputs, with boolean equality as the only primitive and every value of size one as a constant.
// Until something is derived, an argument's options are false, true, x0 and x1, in that order. `unreachable` has the
// same inputs but labels of another type, so no feature can be its output and a session only stops by choice, cap or
// running out of witnesses.
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
    type_id colour;
    const value* red;
    const value* green;
    value_equality<value_pool> equality;
    bin training;
    bin unreachable;
    feature_pool features;
    constant_store constants;
    cheapest_feature_index feature_candidates;
    application_log applications;
    output_finder outputs;
    uniform_value_finder uniform_values;
    value_sizer value_sizes;
    feature_sizer_t feature_sizes;
    primitive_store_t primitives;
    signature_reader_t signatures;
    primitive_caller_t caller;
    variable_candidate_finder_t variable_candidate_finder;
    constant_enumerator_t constant_enumerator;
    constant_seeder_t constant_seeder;
    operand_table_t operand_table;
    operand_applier_t operand_applier;
    app_candidate_finder_t app_candidate_finder;
    witness_detector_t witness_detector;
    primitive_option_finder_t primitive_option_finder;
    argument_option_finder_t argument_option_finder;
    chooser_t chooser;
    argument_chooser_t argument_chooser;
    primitive_applier_t primitive_applier;
    synthesis_starter_t synthesis_starter;
    feature_evaluator<primitive_caller_t> evaluator;
};

FeatureSynthesisIntegrationTest::FeatureSynthesisIntegrationTest()
    : types()
    , values()
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , falsity(values.make(false_ctor, {}))
    , truth(values.make(true_ctor, {}))
    , colour(types.declare_type())
    , red(values.make(types.declare_ctor(colour, {}), {}))
    , green(values.make(types.declare_ctor(colour, {}), {}))
    , equality(values, boolean, boolean_encoding{.type = boolean, .false_ctor = false_ctor, .true_ctor = true_ctor})
    , training{
          .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                    typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
          .labels = {falsity, truth, truth, falsity}}
    , unreachable{.inputs = training.inputs, .labels = {red, green, green, red}}
    , features()
    , constants()
    , feature_candidates()
    , applications()
    , outputs()
    , uniform_values()
    , value_sizes()
    , feature_sizes(value_sizes)
    , primitives(std::vector<primitive>{&equality})
    , signatures(primitives)
    , caller(primitives)
    , variable_candidate_finder(features, feature_sizes, uniform_values)
    , constant_enumerator(types, types, values)
    , constant_seeder(types, constant_enumerator, features, constants)
    , operand_table(constants, constants, feature_candidates, feature_candidates)
    , operand_applier(operand_table, caller)
    , app_candidate_finder(operand_applier, uniform_values, operand_table, features, feature_sizes)
    , witness_detector(operand_table, operand_table, operand_table, applications)
    , primitive_option_finder(primitives, signatures, witness_detector)
    , argument_option_finder(operand_table, operand_table, witness_detector)
    , chooser()
    , argument_chooser(argument_option_finder, chooser)
    , primitive_applier(signatures, argument_chooser, applications, app_candidate_finder, feature_candidates)
    , synthesis_starter(feature_candidates, applications, variable_candidate_finder, feature_candidates)
    , evaluator(caller) {
    constant_seeder.seed(1);
}

feature_synthesizer_t FeatureSynthesisIntegrationTest::make(size_t max_applications) {
    return feature_synthesizer_t{
        synthesis_starter, applications, primitive_option_finder, chooser, primitive_applier, feature_candidates,
        outputs, max_applications};
}

}

TEST_F(FeatureSynthesisIntegrationTest, StoppingAtOnceLeavesTheInputs) {
    feature_synthesizer_t synthesizer = make(10);
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
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
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
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
    const std::vector<candidate>& candidates = synthesizer.synthesize(unreachable);
    const size_t constant_count = constants.constants_of_type(boolean).size();
    const size_t operand_count = constant_count + candidates.size();
    EXPECT_EQ(applications.count(), operand_count * operand_count - constant_count * constant_count);
    EXPECT_GT(candidates.size(), 2u);
}

TEST_F(FeatureSynthesisIntegrationTest, TheSessionAppliesNoMoreThanTheCap) {
    feature_synthesizer_t synthesizer = make(5);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    synthesizer.synthesize(unreachable);
    EXPECT_EQ(applications.count(), 5u);
}

TEST_F(FeatureSynthesisIntegrationTest, EveryDerivedFeatureIsHeterogeneousAndHasItsOwnColumn) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    std::set<column> columns;
    for (const candidate& entry : synthesizer.synthesize(unreachable)) {
        EXPECT_FALSE(uniform_values.find(entry.observed.cells).has_value());
        EXPECT_TRUE(columns.insert(entry.observed.cells).second);
    }
}

TEST_F(FeatureSynthesisIntegrationTest, ANewSessionForgetsTheLastOne) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    synthesizer.synthesize(unreachable);
    ON_CALL(chooser, choose(_)).WillByDefault(Return(0));
    EXPECT_EQ(synthesizer.synthesize(unreachable).size(), 2u);
    EXPECT_EQ(applications.count(), 0u);
}

TEST_F(FeatureSynthesisIntegrationTest, DerivingTheLabelsEndsTheSessionWithThemAsAFeature) {
    feature_synthesizer_t synthesizer = make(10);
    InSequence order;
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(2));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(3));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(5)).WillOnce(Return(4));
    EXPECT_CALL(chooser, choose(5)).WillOnce(Return(0));
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
    const feature* equal_inputs = features.make_app(0, {features.make_var(0), features.make_var(1)});
    const feature* exclusive_or = features.make_app(0, {equal_inputs, features.make_constant(falsity)});
    EXPECT_EQ(outputs.find(candidates, training.labels), std::optional<const feature*>(exclusive_or));
    EXPECT_EQ(applications.count(), 2u);
}

TEST_F(FeatureSynthesisIntegrationTest, AnInputThatIsAlreadyTheOutputStopsTheSessionUnasked) {
    feature_synthesizer_t synthesizer = make(10);
    const bin copy_first{.inputs = training.inputs, .labels = training.inputs[0].cells};
    EXPECT_CALL(chooser, choose(_)).Times(0);
    synthesizer.synthesize(copy_first);
    EXPECT_EQ(applications.count(), 0u);
}

TEST_F(FeatureSynthesisIntegrationTest, EveryFeaturesColumnIsWhatEvaluatingItOnEachRowGives) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    for (const candidate& entry : synthesizer.synthesize(unreachable)) {
        for (size_t row = 0; row < unreachable.labels.size(); ++row) {
            const std::vector<const value*> inputs{unreachable.inputs[0].cells[row], unreachable.inputs[1].cells[row]};
            EXPECT_EQ(evaluator.evaluate(entry.expression, inputs), entry.observed.cells[row]);
        }
    }
}

TEST_F(FeatureSynthesisIntegrationTest, EveryFeaturesCostIsItsSize) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    for (const candidate& entry : synthesizer.synthesize(unreachable))
        EXPECT_EQ(entry.cost, feature_sizes.size(entry.expression));
}
