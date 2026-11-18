#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_manifest.hpp"
#include "infrastructure/value_pool.hpp"

namespace {

// Scout's wiring: the real builder, leaf detector, synthesis, splitter, sizer and evaluator together, choosing the
// first option everywhere.
struct ModelBuildingIntegrationTest : public ::testing::Test {
    ModelBuildingIntegrationTest();
    const value* number(size_t count);
    void expect_fit(const model_node* model, const bin& training);
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
    , manifest(types, values, {}, scout_options{.max_applications = 0, .max_constant_size = 1}) {
    manifest.constant_seeder_.seed(1);
}

const value* ModelBuildingIntegrationTest::number(size_t count) {
    const value* result = values.make(zero_ctor, {});
    for (size_t step = 0; step < count; ++step)
        result = values.make(suc_ctor, {result});
    return result;
}

void ModelBuildingIntegrationTest::expect_fit(const model_node* model, const bin& training) {
    for (size_t row = 0; row < training.labels.size(); ++row) {
        std::vector<const value*> inputs;
        for (const typed_column& input : training.scope)
            inputs.push_back(input.cells[row]);
        EXPECT_EQ(manifest.model_evaluator_.evaluate(model, inputs), std::optional<const value*>(training.labels[row]));
    }
}

}

TEST_F(ModelBuildingIntegrationTest, AgreeingLabelsBuildAConstantLeaf) {
    const bin training{.scope = {typed_column{.type = boolean, .cells = {falsity, truth}}}, .labels = {truth, truth}};
    const model_node* model = manifest.model_builder_.build(training);
    EXPECT_EQ(model, manifest.nodes_.make_leaf(manifest.features_.make_constant(truth)));
    EXPECT_EQ(manifest.model_sizer_.size(model), 1u);
}

TEST_F(ModelBuildingIntegrationTest, AnInputEqualToTheLabelsBuildsAVariableLeaf) {
    const bin training{
        .scope = {typed_column{.type = boolean, .cells = {truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth}}},
        .labels = {falsity, truth}};
    EXPECT_EQ(manifest.model_builder_.build(training), manifest.nodes_.make_leaf(manifest.features_.make_var(1)));
}

TEST_F(ModelBuildingIntegrationTest, NegationIsAMatchWithConstantLeaves) {
    const bin training{.scope = {typed_column{.type = boolean, .cells = {falsity, truth}}}, .labels = {truth, falsity}};
    const model_node* model = manifest.model_builder_.build(training);
    const model_node* expected = manifest.nodes_.make_match(
        manifest.features_.make_var(0),
        {manifest.nodes_.make_leaf(manifest.features_.make_constant(truth)),
         manifest.nodes_.make_leaf(manifest.features_.make_constant(falsity))});
    EXPECT_EQ(model, expected);
    EXPECT_EQ(manifest.model_sizer_.size(model), 4u);
    expect_fit(model, training);
}

TEST_F(ModelBuildingIntegrationTest, AConstructorNoRowHasBecomesAnEmptyBranch) {
    const bin training{
        .scope = {typed_column{.type = nat, .cells = {number(1), number(2)}}}, .labels = {truth, falsity}};
    const model_node* model = manifest.model_builder_.build(training);
    const model_node::match& root = std::get<model_node::match>(model->content);
    EXPECT_EQ(root.branches[0], manifest.nodes_.make_empty());
    EXPECT_FALSE(manifest.model_evaluator_.evaluate(model, {number(0)}).has_value());
    expect_fit(model, training);
}

TEST_F(ModelBuildingIntegrationTest, BuiltModelsFitTheirTrainingData) {
    const bin parity{
        .scope = {typed_column{.type = nat, .cells = {number(0), number(1), number(2), number(3), number(4)}}},
        .labels = {truth, falsity, truth, falsity, truth}};
    const bin conjunction{
        .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = {falsity, falsity, falsity, truth}};
    expect_fit(manifest.model_builder_.build(parity), parity);
    expect_fit(manifest.model_builder_.build(conjunction), conjunction);
}

TEST_F(ModelBuildingIntegrationTest, ConjunctionOutputsTheSecondInputWhereTheFirstIsTrue) {
    const bin training{
        .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                  typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
        .labels = {falsity, falsity, falsity, truth}};
    const model_node* model = manifest.model_builder_.build(training);
    const model_node* expected = manifest.nodes_.make_match(
        manifest.features_.make_var(0),
        {manifest.nodes_.make_leaf(manifest.features_.make_var(0)),
         manifest.nodes_.make_leaf(manifest.features_.make_var(1))});
    EXPECT_EQ(model, expected);
    EXPECT_EQ(manifest.model_sizer_.size(model), 4u);
}

TEST_F(ModelBuildingIntegrationTest, BuildingTheSameDataTwiceGivesTheSameInternedModel) {
    const bin training{.scope = {typed_column{.type = boolean, .cells = {falsity, truth}}}, .labels = {truth, falsity}};
    EXPECT_EQ(manifest.model_builder_.build(training), manifest.model_builder_.build(training));
}
