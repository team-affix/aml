#include <gtest/gtest.h>
#include "value_objects/model_node_hash.hpp"

namespace {

struct ModelNodeHashTest : public ::testing::Test {
    ModelNodeHashTest();
    value falsity;
    value truth;
    feature scrutinee;
    model_node false_leaf;
    model_node true_leaf;
    model_node_hash hasher;
};

ModelNodeHashTest::ModelNodeHashTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , scrutinee{feature::var{.index = 0}}
    , false_leaf{model_node::leaf{.label = &falsity}}
    , true_leaf{model_node::leaf{.label = &truth}}
    , hasher() {}

}

TEST_F(ModelNodeHashTest, EqualMatchesHashEqual) {
    EXPECT_EQ(hasher(model_node{model_node::match{.scrutinee = &scrutinee, .branches = {&false_leaf, &true_leaf}}}),
              hasher(model_node{model_node::match{.scrutinee = &scrutinee, .branches = {&false_leaf, &true_leaf}}}));
}

TEST_F(ModelNodeHashTest, BranchOrderChangesTheHash) {
    EXPECT_NE(hasher(model_node{model_node::match{.scrutinee = &scrutinee, .branches = {&false_leaf, &true_leaf}}}),
              hasher(model_node{model_node::match{.scrutinee = &scrutinee, .branches = {&true_leaf, &false_leaf}}}));
}

TEST_F(ModelNodeHashTest, LeavesWithDifferentLabelsHashApart) {
    EXPECT_NE(hasher(false_leaf), hasher(true_leaf));
}

TEST_F(ModelNodeHashTest, EmptyAndALeafHashApart) {
    EXPECT_NE(hasher(model_node{model_node::empty{}}), hasher(false_leaf));
}
