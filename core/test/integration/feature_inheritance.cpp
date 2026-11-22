#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/application_log.hpp"
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/feature_column_evaluator.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/inherited_feature_store.hpp"
#include "infrastructure/inheriting_bin_splitter.hpp"
#include "infrastructure/inheriting_synthesis_starter.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/synthesis_starter.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "support/nat_add.hpp"
#include "value_objects/bin_view.hpp"
#include "value_objects/typed_primitive.hpp"

using namespace AML;

namespace {

using feature_sizer_t = feature_sizer<value_sizer>;
using variable_candidate_finder_t = variable_candidate_finder<feature_sizer_t, uniform_value_finder>;
using primitive_caller_t = primitive_caller<primitive_store>;
using feature_column_evaluator_t = feature_column_evaluator<primitive_store, primitive_caller_t>;
using synthesis_starter_t =
    synthesis_starter<cheapest_feature_index, application_log, variable_candidate_finder_t, cheapest_feature_index>;
using inheriting_synthesis_starter_t = inheriting_synthesis_starter<
    synthesis_starter_t, inherited_feature_store, feature_column_evaluator_t, uniform_value_finder, feature_sizer_t,
    cheapest_feature_index>;
using inheriting_bin_splitter_t = inheriting_bin_splitter<
    bin_splitter<schema>, cheapest_feature_index, inherited_feature_store, inherited_feature_store,
    inherited_feature_store, inherited_feature_store>;

// Peano naturals; the scrutinee is add(x0, x0), so over x0 = 0, 1, 2 it reads 0, 2, 4, and its zero child holds the
// row with x0 = 0 and its successor child the rows with x0 = 1 and 2.
struct FeatureInheritanceIntegrationTest : public ::testing::Test {
    FeatureInheritanceIntegrationTest();
    value natural(size_t number) const;
    bool holds(const feature& expression, const column_view& expected) const;
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
    feature_column_evaluator_t evaluator;
    value_sizer value_sizer_;
    feature_sizer_t sizer;
    uniform_value_finder uniform;
    variable_candidate_finder_t variables;
    cheapest_feature_index features;
    application_log applications;
    synthesis_starter_t plain_starter;
    inherited_feature_store inherited;
    inheriting_synthesis_starter_t starter;
    bin_splitter<schema> plain_splitter;
    inheriting_bin_splitter_t splitter;
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
    , evaluator(primitives, caller)
    , value_sizer_()
    , sizer(value_sizer_)
    , uniform()
    , variables(sizer, uniform)
    , features()
    , applications()
    , plain_starter(features, applications, variables, features)
    , inherited()
    , starter(plain_starter, inherited, evaluator, uniform, sizer, features)
    , plain_splitter(types)
    , splitter(plain_splitter, features, inherited, inherited, inherited, inherited) {}

value FeatureInheritanceIntegrationTest::natural(size_t number) const {
    value term{.ctor = zero_ctor, .args = {}};
    for (size_t step = 0; step < number; ++step)
        term = value{.ctor = suc_ctor, .args = {term}};
    return term;
}

bool FeatureInheritanceIntegrationTest::holds(const feature& expression, const column_view& expected) const {
    for (const candidate& held : features.candidates()) {
        if (held.expression == expression && held.observed == expected)
            return true;
    }
    return false;
}

}

TEST_F(FeatureInheritanceIntegrationTest, ASessionBelowAMatchHoldsTheScrutineeOverItsRows) {
    starter.start(root);
    features.try_insert(
        candidate{.expression = doubled, .cost = sizer.size(doubled), .observed = evaluator.evaluate(doubled, root)});
    const std::vector<bin_view> children = splitter.split(root, features.candidates().back().observed);
    starter.start(children[1]);
    const column_view expected = column_view::of(typed_column{.type = nat, .cells = {natural(2), natural(4)}});
    EXPECT_TRUE(holds(doubled, expected));
}

TEST_F(FeatureInheritanceIntegrationTest, AnInheritedFeatureUniformOverAChildIsLeftOut) {
    starter.start(root);
    features.try_insert(
        candidate{.expression = doubled, .cost = sizer.size(doubled), .observed = evaluator.evaluate(doubled, root)});
    const std::vector<bin_view> children = splitter.split(root, features.candidates().back().observed);
    starter.start(children[0]);
    for (const candidate& held : features.candidates())
        EXPECT_NE(held.expression, doubled);
}

TEST_F(FeatureInheritanceIntegrationTest, TheRootsSessionInheritsNothing) {
    starter.start(root);
    EXPECT_EQ(features.candidates().size(), 1u);
    EXPECT_EQ(features.candidates()[0].expression, x0);
}

TEST_F(FeatureInheritanceIntegrationTest, InheritanceCarriesDownThroughAMatchOnABoundArgument) {
    starter.start(root);
    features.try_insert(
        candidate{.expression = doubled, .cost = sizer.size(doubled), .observed = evaluator.evaluate(doubled, root)});
    const std::vector<bin_view> children = splitter.split(root, features.candidates().back().observed);
    // The successor child binds add(x0, x0) - 1 as x1 (1 and 3); splitting on it passes nothing new.
    const std::vector<bin_view> grandchildren = splitter.split(children[1], children[1].inputs[1]);
    starter.start(grandchildren[1]);
    const column_view expected = column_view::of(typed_column{.type = nat, .cells = {natural(2), natural(4)}});
    EXPECT_TRUE(holds(doubled, expected));
}

TEST_F(FeatureInheritanceIntegrationTest, SplittingTheRootAgainForgetsTheLastModelsInheritance) {
    starter.start(root);
    features.try_insert(
        candidate{.expression = doubled, .cost = sizer.size(doubled), .observed = evaluator.evaluate(doubled, root)});
    const std::vector<bin_view> children = splitter.split(root, features.candidates().back().observed);
    starter.start(root);
    const std::vector<bin_view> next_children = splitter.split(root, root.inputs[0]);
    EXPECT_FALSE(inherited.has(children[1].labels.rows.get()));
    EXPECT_TRUE(inherited.inherited(next_children[1].labels.rows.get()).empty());
}
