#include <set>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_manifest.hpp"
#include "infrastructure/value_equality.hpp"
#include "infrastructure/value_pool.hpp"

namespace {

// XOR over two boolean inputs, with boolean equality as the only primitive and the booleans as constants.
struct FeatureSynthesisIntegrationTest : public ::testing::Test {
    FeatureSynthesisIntegrationTest();
    schema types;
    value_pool values;
    type_id boolean;
    ctor_id false_ctor;
    ctor_id true_ctor;
    const value* falsity;
    const value* truth;
    value_equality<value_pool> equality;
    scout_manifest manifest;
    bin training;
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
    , manifest(types, values, std::vector<primitive>{&equality},
               scout_options{.max_synthesis_depth = 1, .max_constant_size = 1})
    , training{
          .scope = {typed_column{.type = boolean, .cells = {falsity, falsity, truth, truth}},
                    typed_column{.type = boolean, .cells = {falsity, truth, falsity, truth}}},
          .labels = {falsity, truth, truth, falsity}} {
    manifest.constant_seeder_.seed(1);
}

}

TEST_F(FeatureSynthesisIntegrationTest, TheInputsComeFirstAtCostOne) {
    const std::vector<candidate> candidates = manifest.feature_synthesizer_.synthesize(training);
    ASSERT_GE(candidates.size(), 2u);
    EXPECT_EQ(candidates[0].expression, manifest.features_.make_var(0));
    EXPECT_EQ(candidates[1].expression, manifest.features_.make_var(1));
    EXPECT_EQ(candidates[0].cost, 1u);
}

TEST_F(FeatureSynthesisIntegrationTest, EqualityOfTheInputsIsSynthesizedAtCostThree) {
    const feature* equal_inputs =
        manifest.features_.make_app(0, {manifest.features_.make_var(0), manifest.features_.make_var(1)});
    const std::vector<candidate> candidates = manifest.feature_synthesizer_.synthesize(training);
    bool found = false;
    for (const candidate& entry : candidates) {
        if (entry.expression != equal_inputs)
            continue;
        found = true;
        EXPECT_EQ(entry.cost, 3u);
        EXPECT_EQ(entry.observed.cells, (column{truth, falsity, falsity, truth}));
    }
    EXPECT_TRUE(found);
}

TEST_F(FeatureSynthesisIntegrationTest, EveryCandidateIsHeterogeneousAndHasItsOwnColumn) {
    std::set<column> columns;
    for (const candidate& entry : manifest.feature_synthesizer_.synthesize(training)) {
        EXPECT_FALSE(manifest.uniform_value_finder_.find(entry.observed.cells).has_value());
        EXPECT_TRUE(columns.insert(entry.observed.cells).second);
    }
}
