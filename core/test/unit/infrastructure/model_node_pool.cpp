#include <gtest/gtest.h>
#include "infrastructure/model_node_pool.hpp"

namespace {

struct ModelNodePoolTest : public ::testing::Test {
    ModelNodePoolTest();
    value falsity;
    feature scrutinee;
    model_node_pool nodes;
};

ModelNodePoolTest::ModelNodePoolTest()
    : falsity{.ctor = 0, .fields = {}}
    , scrutinee{feature::var{.index = 0}}
    , nodes() {}

}

TEST_F(ModelNodePoolTest, EqualLeavesShareOnePointer) {
    EXPECT_EQ(nodes.make_leaf(&falsity), nodes.make_leaf(&falsity));
}

TEST_F(ModelNodePoolTest, EmptyNodesShareOnePointer) {
    EXPECT_EQ(nodes.make_empty(), nodes.make_empty());
}

TEST_F(ModelNodePoolTest, EqualMatchesShareOnePointer) {
    const model_node* leaf = nodes.make_leaf(&falsity);
    const model_node* empty = nodes.make_empty();
    EXPECT_EQ(nodes.make_match(&scrutinee, {leaf, empty}), nodes.make_match(&scrutinee, {leaf, empty}));
    EXPECT_NE(nodes.make_match(&scrutinee, {leaf, empty}), nodes.make_match(&scrutinee, {empty, leaf}));
}

TEST_F(ModelNodePoolTest, AMadeMatchHoldsItsScrutineeAndBranches) {
    const model_node* leaf = nodes.make_leaf(&falsity);
    EXPECT_EQ(*nodes.make_match(&scrutinee, {leaf}),
              (model_node{model_node::match{.scrutinee = &scrutinee, .branches = {leaf}}}));
}
