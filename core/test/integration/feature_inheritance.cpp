#include <cstddef>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/feature_column_evaluator.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/inheriting_bin_splitter.hpp"
#include "infrastructure/inheriting_model_evaluator.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scrutinee_piece_finder.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "support/nat_add.hpp"
#include "value_objects/bin_view.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

using feature_sizer_t = feature_sizer<value_sizer>;
using variable_candidate_finder_t = variable_candidate_finder<feature_sizer_t, uniform_value_finder>;
using primitive_caller_t = primitive_caller<primitive_store>;
using feature_column_evaluator_t = feature_column_evaluator<primitive_store, primitive_caller_t>;
using inheriting_bin_splitter_t = inheriting_bin_splitter<
    bin_splitter<schema>, cheapest_feature_index, scrutinee_piece_finder, feature_column_evaluator_t>;
using feature_evaluator_t = feature_evaluator<primitive_caller_t>;
using inheriting_model_evaluator_t = inheriting_model_evaluator<feature_evaluator_t, scrutinee_piece_finder>;

// Peano naturals and x0 = 0, 1, 2. The scrutinee is add(x0, x0), reading 0, 2, 4: its zero child holds the row with
// x0 = 0 (variables x0, then the piece); its successor child the rows with x0 = 1 and 2 (variables x0, the bound
// argument add(x0, x0) - 1, then the piece).
struct FeatureInheritanceIntegrationTest : public ::testing::Test {
    FeatureInheritanceIntegrationTest();
    value natural(size_t number) const;
    column_view naturals(const std::vector<size_t>& numbers) const;
    std::vector<bin_view> split_by_doubled();
    schema types;
    type_id nat;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    feature x0;
    feature doubled;
    bin data;
    bin_view root;
    primitive_store primitives;
    primitive_caller_t caller;
    feature_column_evaluator_t column_evaluator;
    value_sizer value_sizer_;
    feature_sizer_t sizer;
    uniform_value_finder uniform;
    variable_candidate_finder_t variables;
    cheapest_feature_index features;
    scrutinee_piece_finder pieces;
    bin_splitter<schema> plain_splitter;
    inheriting_bin_splitter_t splitter;
    feature_evaluator_t evaluator;
    inheriting_model_evaluator_t model_evaluator;
};

FeatureInheritanceIntegrationTest::FeatureInheritanceIntegrationTest()
    : types()
    , nat(types.declare_type())
    , zero_ctor(types.declare_ctor(nat, {}))
    , suc_ctor(types.declare_ctor(nat, {nat}))
    , x0{feature::var{.index = 0}}
    , doubled{feature::app{.prim = 0, .args = {x0, x0}}}
    , data{.inputs = {typed_column{.type = nat, .cells = {natural(0), natural(1), natural(2)}}},
           .labels = typed_column{.type = nat, .cells = {natural(0), natural(1), natural(2)}}}
    , root(bin_view::of(data))
    , primitives({typed_primitive{.type = primitive_type{.arg_types = {nat, nat}, .result_type = nat},
                                  .apply = nat_add(suc_ctor)}})
    , caller(primitives)
    , column_evaluator(primitives, caller)
    , value_sizer_()
    , sizer(value_sizer_)
    , uniform()
    , variables(sizer, uniform)
    , features()
    , pieces()
    , plain_splitter(types)
    , splitter(plain_splitter, features, pieces, column_evaluator)
    , evaluator(caller)
    , model_evaluator(evaluator, pieces) {}

value FeatureInheritanceIntegrationTest::natural(size_t number) const {
    value term{.ctor = zero_ctor, .args = {}};
    for (size_t step = 0; step < number; ++step)
        term = value{.ctor = suc_ctor, .args = {term}};
    return term;
}

column_view FeatureInheritanceIntegrationTest::naturals(const std::vector<size_t>& numbers) const {
    column cells;
    for (const size_t number : numbers)
        cells.push_back(natural(number));
    return column_view::of(typed_column{.type = nat, .cells = std::move(cells)});
}

std::vector<bin_view> FeatureInheritanceIntegrationTest::split_by_doubled() {
    features.try_insert(candidate{.expression = doubled, .cost = sizer.size(doubled),
                                  .observed = column_evaluator.evaluate(doubled, root)});
    return splitter.split(root, features.candidates().back().observed);
}

}

TEST_F(FeatureInheritanceIntegrationTest, AChildHoldsTheScrutineeAsAVariableAfterItsBoundArguments) {
    const std::vector<bin_view> children = split_by_doubled();
    ASSERT_EQ(children[1].inputs.size(), 3u);
    EXPECT_EQ(children[1].inputs[1], naturals({1, 3}));
    EXPECT_EQ(children[1].inputs[2], naturals({2, 4}));
}

TEST_F(FeatureInheritanceIntegrationTest, APieceUniformOverAChildStaysButIsNotOffered) {
    const std::vector<bin_view> children = split_by_doubled();
    ASSERT_EQ(children[0].inputs.size(), 2u);
    EXPECT_EQ(children[0].inputs[1], naturals({0}));
    EXPECT_TRUE(variables.find(children[0], 0).empty());
}

TEST_F(FeatureInheritanceIntegrationTest, AHeterogeneousPieceIsOfferedAsAVariableCostingOne) {
    const std::vector<bin_view> children = split_by_doubled();
    const std::vector<candidate> offered = variables.find(children[1], 0);
    ASSERT_EQ(offered.size(), 3u);
    EXPECT_EQ(offered[2].expression, (feature{feature::var{.index = 2}}));
    EXPECT_EQ(offered[2].cost, 1u);
    EXPECT_EQ(offered[2].observed, naturals({2, 4}));
}

TEST_F(FeatureInheritanceIntegrationTest, APieceCarriesDownThroughAMatchOnABoundArgument) {
    const std::vector<bin_view> children = split_by_doubled();
    const std::vector<bin_view> grandchildren = splitter.split(children[1], children[1].inputs[1]);
    ASSERT_EQ(grandchildren[1].inputs.size(), 4u);
    EXPECT_EQ(grandchildren[1].inputs[2], naturals({2, 4}));
}

TEST_F(FeatureInheritanceIntegrationTest, InferenceNumbersThePiecesAsTrainingDoes) {
    const std::vector<bin_view> children = split_by_doubled();
    const model_node model{model_node::match{
        .scrutinee = doubled,
        .branches = {model_node{model_node::empty{}},
                     model_node{model_node::leaf{.output = feature{feature::var{.index = 2}}}}}}};
    const column_view& piece = children[1].inputs[2];
    const std::vector<size_t> inputs{1, 2};
    for (size_t row = 0; row < inputs.size(); ++row) {
        const value expected = (*piece.cells)[(*piece.rows)[row]];
        EXPECT_EQ(model_evaluator.evaluate(model, {natural(inputs[row])}), std::optional<value>(expected));
    }
}
