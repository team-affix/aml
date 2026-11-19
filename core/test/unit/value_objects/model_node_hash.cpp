#include <gtest/gtest.h>
#include "value_objects/model_node_hash.hpp"

namespace {

struct ModelNodeHashTest : public ::testing::Test {
    ModelNodeHashTest();
    value falsity;
    value truth;
    feature scrutinee;
    feature other_scrutinee;
    feature false_output;
    feature true_output;
    model_node false_leaf;
    model_node true_leaf;
    model_node_hash hasher;
};

ModelNodeHashTest::ModelNodeHashTest()
    : falsity{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , truth{.ctor = {.type = 0, .tag = 1}, .args = {}}
    , scrutinee{feature::var{.index = 0}}
    , other_scrutinee{feature::var{.index = 1}}
    , false_output{feature::constant{.literal = &falsity}}
    , true_output{feature::constant{.literal = &truth}}
    , false_leaf{model_node::leaf{.output = &false_output}}
    , true_leaf{model_node::leaf{.output = &true_output}}
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

TEST_F(ModelNodeHashTest, TheScrutineeChangesTheHash) {
    EXPECT_NE(hasher(model_node{model_node::match{.scrutinee = &scrutinee, .branches = {&false_leaf}}}),
              hasher(model_node{model_node::match{.scrutinee = &other_scrutinee, .branches = {&false_leaf}}}));
}

TEST_F(ModelNodeHashTest, EqualLeavesHashEqual) {
    EXPECT_EQ(hasher(false_leaf), hasher(model_node{model_node::leaf{.output = &false_output}}));
}

TEST_F(ModelNodeHashTest, LeavesWithDifferentOutputsHashApart) {
    EXPECT_NE(hasher(false_leaf), hasher(true_leaf));
}

TEST_F(ModelNodeHashTest, EmptyAndALeafHashApart) {
    EXPECT_NE(hasher(model_node{model_node::empty{}}), hasher(false_leaf));
}

TEST_F(ModelNodeHashTest, EmptiesHashEqual) {
    EXPECT_EQ(hasher(model_node{model_node::empty{}}), hasher(model_node{model_node::empty{}}));
}
