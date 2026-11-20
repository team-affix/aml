#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_chooser.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/operand_applier.hpp"
#include "infrastructure/operand_table.hpp"
#include "infrastructure/output_finder.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/primitive_applier.hpp"
#include "infrastructure/primitive_option_finder.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/synthesis_starter.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/witness_detector.hpp"
#include "support/value_equality.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

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
using primitive_caller_t = primitive_caller<primitive_store>;
using variable_candidate_finder_t = variable_candidate_finder<feature_sizer_t, uniform_value_finder>;
using constant_seeder_t = constant_seeder<constant_store>;
using operand_table_t =
    operand_table<constant_store, constant_store, cheapest_feature_index, cheapest_feature_index>;
using operand_applier_t = operand_applier<operand_table_t, primitive_caller_t>;
using app_candidate_finder_t =
    app_candidate_finder<operand_applier_t, uniform_value_finder, operand_table_t, feature_sizer_t>;
using witness_detector_t = witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log>;
using primitive_option_finder_t =
    primitive_option_finder<primitive_store, primitive_store, witness_detector_t>;
using argument_option_finder_t =
    argument_option_finder<operand_table_t, operand_table_t, witness_detector_t>;
using argument_chooser_t = argument_chooser<argument_option_finder_t, chooser_t>;
using primitive_applier_t = primitive_applier<
    primitive_store, argument_chooser_t, application_log, app_candidate_finder_t, cheapest_feature_index>;
using synthesis_starter_t =
    synthesis_starter<cheapest_feature_index, application_log, variable_candidate_finder_t, cheapest_feature_index>;
using feature_synthesizer_t = feature_synthesizer<
    synthesis_starter_t, application_log, primitive_option_finder_t, chooser_t, chooser_t, primitive_applier_t,
    cheapest_feature_index, output_finder>;

// XOR over two boolean inputs, with boolean equality as the only primitive and false, true, red and green as constants.
// Each step is a choice between stopping and continuing (2 options); continuing then chooses the primitive among the
// one there is (1 option). Until something is derived, an argument's options are false, true, x0 and x1, in that
// order. `unreachable` has the
// same inputs but labels of another type, so no feature can be its output and a session only stops by choice, cap or
// running out of witnesses.
struct FeatureSynthesisIntegrationTest : public ::testing::Test {
    FeatureSynthesisIntegrationTest();
    feature_synthesizer_t make(size_t max_applications);
    schema types;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    value falsity;
    value truth;
    type_id colour;
    value red;
    value green;
    typed_primitive equality;
    bin training;
    bin unreachable;
    feature x0;
    feature x1;
    constant_store constants;
    cheapest_feature_index feature_candidates;
    application_log applications;
    output_finder outputs;
    uniform_value_finder uniform_values;
    value_sizer value_sizes;
    feature_sizer_t feature_sizes;
    primitive_store primitives;
    primitive_caller_t caller;
    variable_candidate_finder_t variable_candidate_finder;
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
    , boolean(types.declare_type())
    , false_ctor(types.declare_ctor(boolean, {}))
    , true_ctor(types.declare_ctor(boolean, {}))
    , falsity(value{.ctor = false_ctor, .args = {}})
    , truth(value{.ctor = true_ctor, .args = {}})
    , colour(types.declare_type())
    , red(value{.ctor = types.declare_ctor(colour, {}), .args = {}})
    , green(value{.ctor = types.declare_ctor(colour, {}), .args = {}})
    , equality{
          .type = primitive_type{.arg_types = {boolean, boolean}, .result_type = boolean},
          .apply = value_equality(false_ctor, true_ctor)}
    , training{
          .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                    typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
          .labels = typed_column{.type = boolean, .cells = {falsity, truth, truth, falsity}}}
    , unreachable{.inputs = training.inputs, .labels = typed_column{.type = colour, .cells = {red, green, green, red}}}
    , x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , constants()
    , feature_candidates()
    , applications()
    , outputs()
    , uniform_values()
    , value_sizes()
    , feature_sizes(value_sizes)
    , primitives(std::vector<typed_primitive>{equality})
    , caller(primitives)
    , variable_candidate_finder(feature_sizes, uniform_values)
    , constant_seeder(constants)
    , operand_table(constants, constants, feature_candidates, feature_candidates)
    , operand_applier(operand_table, caller)
    , app_candidate_finder(operand_applier, uniform_values, operand_table, feature_sizes)
    , witness_detector(operand_table, operand_table, operand_table, applications)
    , primitive_option_finder(primitives, primitives, witness_detector)
    , argument_option_finder(operand_table, operand_table, witness_detector)
    , chooser()
    , argument_chooser(argument_option_finder, chooser)
    , primitive_applier(primitives, argument_chooser, applications, app_candidate_finder, feature_candidates)
    , synthesis_starter(feature_candidates, applications, variable_candidate_finder, feature_candidates)
    , evaluator(caller) {
    constant_seeder.seed(std::vector<typed_constant>{
        typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
        typed_constant{.type = colour, .literal = red}, typed_constant{.type = colour, .literal = green}});
}

feature_synthesizer_t FeatureSynthesisIntegrationTest::make(size_t max_applications) {
    return feature_synthesizer_t{
        synthesis_starter, applications, primitive_option_finder, chooser, chooser, primitive_applier,
        feature_candidates, outputs, max_applications};
}

}

TEST_F(FeatureSynthesisIntegrationTest, StoppingAtOnceLeavesTheInputs) {
    feature_synthesizer_t synthesizer = make(10);
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
    ASSERT_EQ(candidates.size(), 2u);
    EXPECT_EQ(candidates[0].expression, x0);
    EXPECT_EQ(candidates[1].expression, x1);
}

TEST_F(FeatureSynthesisIntegrationTest, ChoosingEqualityOfTheInputsDerivesItAtCostThree) {
    feature_synthesizer_t synthesizer = make(10);
    InSequence order;
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(1)).WillOnce(Return(0));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(2));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(3));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(0));
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
    ASSERT_EQ(candidates.size(), 3u);
    EXPECT_EQ(candidates[2].expression, (feature{feature::app{.prim = 0, .args = {x0, x1}}}));
    EXPECT_EQ(candidates[2].cost, 3u);
    EXPECT_EQ(candidates[2].observed.cells, (column{truth, falsity, falsity, truth}));
    EXPECT_EQ(applications.count(), 1u);
}

TEST_F(FeatureSynthesisIntegrationTest, ATupleOfConstantsIsNeverOffered) {
    feature_synthesizer_t synthesizer = make(10);
    InSequence order;
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(1)).WillOnce(Return(0));
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
    std::vector<column> columns;
    for (const candidate& entry : synthesizer.synthesize(unreachable)) {
        EXPECT_FALSE(uniform_values.find(entry.observed.cells).has_value());
        EXPECT_EQ(std::find(columns.begin(), columns.end(), entry.observed.cells), columns.end());
        columns.push_back(entry.observed.cells);
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
    EXPECT_CALL(chooser, choose(1)).WillOnce(Return(0));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(2));
    EXPECT_CALL(chooser, choose(4)).WillOnce(Return(3));
    EXPECT_CALL(chooser, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(chooser, choose(1)).WillOnce(Return(0));
    EXPECT_CALL(chooser, choose(5)).WillOnce(Return(4));
    EXPECT_CALL(chooser, choose(5)).WillOnce(Return(0));
    const std::vector<candidate>& candidates = synthesizer.synthesize(training);
    const feature equal_inputs{feature::app{.prim = 0, .args = {x0, x1}}};
    const feature false_constant{feature::constant{.literal = falsity}};
    const feature exclusive_or{feature::app{.prim = 0, .args = {equal_inputs, false_constant}}};
    EXPECT_EQ(outputs.find(candidates, training.labels), std::optional<feature>(exclusive_or));
    EXPECT_EQ(applications.count(), 2u);
}

TEST_F(FeatureSynthesisIntegrationTest, AnInputThatIsAlreadyTheOutputStopsTheSessionUnasked) {
    feature_synthesizer_t synthesizer = make(10);
    const bin copy_first{.inputs = training.inputs, .labels = training.inputs[0]};
    EXPECT_CALL(chooser, choose(_)).Times(0);
    synthesizer.synthesize(copy_first);
    EXPECT_EQ(applications.count(), 0u);
}

TEST_F(FeatureSynthesisIntegrationTest, EveryFeaturesColumnIsWhatEvaluatingItOnEachRowGives) {
    feature_synthesizer_t synthesizer = make(1000);
    ON_CALL(chooser, choose(_)).WillByDefault(Invoke(&last_option));
    for (const candidate& entry : synthesizer.synthesize(unreachable)) {
        for (size_t row = 0; row < unreachable.labels.cells.size(); ++row) {
            const std::vector<value> inputs{unreachable.inputs[0].cells[row], unreachable.inputs[1].cells[row]};
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
