#include <gtest/gtest.h>
#include "infrastructure/model_node_pool.hpp"

namespace {

struct ModelNodePoolTest : public ::testing::Test {
    ModelNodePoolTest();
    value falsity;
    feature scrutinee;
    feature false_output;
    feature variable_output;
    model_node_pool nodes;
};

ModelNodePoolTest::ModelNodePoolTest()
    : falsity{.tag = 0, .args = {}}
    , scrutinee{feature::var{.index = 0}}
    , false_output{feature::constant{.literal = &falsity}}
    , variable_output{feature::var{.index = 1}}
    , nodes() {}

}

TEST_F(ModelNodePoolTest, EqualLeavesShareOnePointer) {
    EXPECT_EQ(nodes.make_leaf(&false_output), nodes.make_leaf(&false_output));
}

TEST_F(ModelNodePoolTest, LeavesWithDifferentOutputsAreDistinct) {
    EXPECT_NE(nodes.make_leaf(&false_output), nodes.make_leaf(&variable_output));
}

TEST_F(ModelNodePoolTest, AMadeLeafHoldsItsOutput) {
    EXPECT_EQ(*nodes.make_leaf(&variable_output), (model_node{model_node::leaf{.output = &variable_output}}));
}

TEST_F(ModelNodePoolTest, EmptyNodesShareOnePointer) {
    EXPECT_EQ(nodes.make_empty(), nodes.make_empty());
}

TEST_F(ModelNodePoolTest, EqualMatchesShareOnePointer) {
    const model_node* leaf = nodes.make_leaf(&false_output);
    const model_node* empty = nodes.make_empty();
    EXPECT_EQ(nodes.make_match(&scrutinee, {leaf, empty}), nodes.make_match(&scrutinee, {leaf, empty}));
    EXPECT_NE(nodes.make_match(&scrutinee, {leaf, empty}), nodes.make_match(&scrutinee, {empty, leaf}));
}

TEST_F(ModelNodePoolTest, AMadeMatchHoldsItsScrutineeAndBranches) {
    const model_node* leaf = nodes.make_leaf(&false_output);
    EXPECT_EQ(*nodes.make_match(&scrutinee, {leaf}),
              (model_node{model_node::match{.scrutinee = &scrutinee, .branches = {leaf}}}));
}

TEST_F(ModelNodePoolTest, NodesOfDifferentKindsAreDistinct) {
    EXPECT_NE(nodes.make_empty(), nodes.make_leaf(&false_output));
}
