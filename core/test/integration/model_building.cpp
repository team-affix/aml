#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_manifest.hpp"
#include "infrastructure/value_pool.hpp"
#include "value_objects/typed_constant.hpp"

using namespace AML;

namespace {

// Scout's wiring: the real builder, leaf detector, synthesis, splitter, sizer and evaluator together, choosing the
// first option everywhere.
struct ModelBuildingIntegrationTest : public ::testing::Test {
    ModelBuildingIntegrationTest();
    const value* number(size_t count);
    void expect_fit(const model_node& model, const bin& training);
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
    scout_manifest manifest;
};

ModelBuildingIntegrationTest::ModelBuildingIntegrationTest()
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
    , manifest(types, values, {}, scout_options{.max_applications = 0}) {
    manifest.constant_seeder_.seed(std::vector<typed_constant>{
        typed_constant{.type = boolean, .literal = falsity}, typed_constant{.type = boolean, .literal = truth},
        typed_constant{.type = nat, .literal = values.make(zero_ctor, {})}});
}

const value* ModelBuildingIntegrationTest::number(size_t count) {
    const value* result = values.make(zero_ctor, {});
    for (size_t step = 0; step < count; ++step)
        result = values.make(suc_ctor, {result});
    return result;
}

void ModelBuildingIntegrationTest::expect_fit(const model_node& model, const bin& training) {
    for (size_t row = 0; row < training.labels.cells.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.inputs)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(manifest.model_evaluator_.evaluate(model, inputs),
                  std::optional<const value*>(training.labels.cells[row]));
    }
}

}

TEST_F(ModelBuildingIntegrationTest, AgreeingLabelsBuildAConstantLeaf) {
    const bin training{.inputs = {typed_column{.type = boolean, .cells = {falsity, truth}}},
                       .labels = typed_column{.type = boolean, .cells = {truth, truth}}};
    const model_node model = manifest.model_builder_.build(training);
    EXPECT_EQ(model, (model_node{model_node::leaf{.output = feature{feature::constant{.literal = truth}}}}));
    EXPECT_EQ(manifest.model_sizer_.size(model), 1u);
}

TEST_F(ModelBuildingIntegrationTest, AnInputEqualToTheLabelsBuildsAVariableLeaf) {
    const bin training{
        .inputs = {typed_column{.type = boolean, .cells = {truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, truth}}};
    EXPECT_EQ(manifest.model_builder_.build(training),
              (model_node{model_node::leaf{.output = feature{feature::var{.index = 1}}}}));
}

TEST_F(ModelBuildingIntegrationTest, NegationIsAMatchWithConstantLeaves) {
    const bin training{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {truth, falsity}}};
    const model_node model = manifest.model_builder_.build(training);
    const model_node expected{model_node::match{
        .scrutinee = feature{feature::var{.index = 0}},
        .branches = {
            model_node{model_node::leaf{.output = feature{feature::constant{.literal = truth}}}},
            model_node{model_node::leaf{.output = feature{feature::constant{.literal = falsity}}}}}}};
    EXPECT_EQ(model, expected);
    EXPECT_EQ(manifest.model_sizer_.size(model), 4u);
    expect_fit(model, training);
}

TEST_F(ModelBuildingIntegrationTest, AConstructorNoRowHasBecomesAnEmptyBranch) {
    const bin training{
        .inputs = {typed_column{.type = nat, .cells = {number(1), number(2)}}},
        .labels = typed_column{.type = boolean, .cells = {truth, falsity}}};
    const model_node model = manifest.model_builder_.build(training);
    const model_node::match& root = std::get<model_node::match>(model.content);
    EXPECT_EQ(root.branches[0], (model_node{model_node::empty{}}));
    EXPECT_FALSE(manifest.model_evaluator_.evaluate(model, {number(0)}).has_value());
    expect_fit(model, training);
}

TEST_F(ModelBuildingIntegrationTest, BuiltModelsFitTheirTrainingData) {
    const bin parity{
        .inputs = {typed_column{.type = nat, .cells = {number(0), number(1), number(2), number(3), number(4)}}},
        .labels = typed_column{.type = boolean, .cells = {truth, falsity, truth, falsity, truth}}};
    const bin conjunction{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, falsity, falsity, truth}}};
    expect_fit(manifest.model_builder_.build(parity), parity);
    expect_fit(manifest.model_builder_.build(conjunction), conjunction);
}

TEST_F(ModelBuildingIntegrationTest, ConjunctionOutputsTheSecondInputWhereTheFirstIsTrue) {
    const bin training{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {falsity, falsity, falsity, truth}}};
    const model_node model = manifest.model_builder_.build(training);
    const model_node expected{model_node::match{
        .scrutinee = feature{feature::var{.index = 0}},
        .branches = {
            model_node{model_node::leaf{.output = feature{feature::var{.index = 0}}}},
            model_node{model_node::leaf{.output = feature{feature::var{.index = 1}}}}}}};
    EXPECT_EQ(model, expected);
    EXPECT_EQ(manifest.model_sizer_.size(model), 4u);
}

TEST_F(ModelBuildingIntegrationTest, BuildingTheSameDataTwiceGivesEqualModels) {
    const bin training{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, truth}}},
        .labels = typed_column{.type = boolean, .cells = {truth, falsity}}};
    EXPECT_EQ(manifest.model_builder_.build(training), manifest.model_builder_.build(training));
}

TEST_F(ModelBuildingIntegrationTest, AnAgreeingLabelNeedNotBeASeededConstant) {
    const bin training{
        .inputs = {typed_column{.type = boolean, .cells = {falsity, truth}}},
        .labels = typed_column{.type = nat, .cells = {number(5), number(5)}}};
    const model_node model = manifest.model_builder_.build(training);
    // Only zero is seeded at size one, yet the leaf outputs five.
    EXPECT_EQ(manifest.constants_.constants_of_type(nat).size(), 1u);
    EXPECT_EQ(model, (model_node{model_node::leaf{.output = feature{feature::constant{.literal = number(5)}}}}));
    EXPECT_EQ(manifest.model_sizer_.size(model), 6u);
    expect_fit(model, training);
}
