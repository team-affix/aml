#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_evaluator.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockEvaluateFeature {
    MOCK_METHOD(const value*, evaluate, (const feature*, const std::vector<const value*>&), (const));
};

struct MockGetCtorDecl {
    MOCK_METHOD(const ctor_decl&, decl, (ctor_id), (const));
};

// Nat: zero = 0, suc = 1. The outer match is on x0: zero gives false; suc binds x1 and matches on it, where zero
// gives nothing and suc (binding x2) outputs x2.
struct ModelEvaluatorTest : public ::testing::Test {
    ModelEvaluatorTest();
    ctor_decl zero_decl;
    ctor_decl suc_decl;
    value zero;
    value one;
    value two;
    value falsity;
    feature input;
    feature predecessor;
    feature second_predecessor;
    feature false_output;
    model_node empty;
    model_node false_leaf;
    model_node variable_leaf;
    model_node inner;
    model_node outer;
    NiceMock<MockEvaluateFeature> evaluate_feature;
    NiceMock<MockGetCtorDecl> get_ctor_decl;
    model_evaluator<NiceMock<MockEvaluateFeature>, NiceMock<MockGetCtorDecl>> evaluator{
        evaluate_feature, get_ctor_decl};

    void SetUp() override {
        ON_CALL(get_ctor_decl, decl(0)).WillByDefault(ReturnRef(zero_decl));
        ON_CALL(get_ctor_decl, decl(1)).WillByDefault(ReturnRef(suc_decl));
        ON_CALL(evaluate_feature, evaluate(&false_output, _)).WillByDefault(Return(&falsity));
    }
};

ModelEvaluatorTest::ModelEvaluatorTest()
    : zero_decl{.type = 0, .tag = 0, .field_types = {}}
    , suc_decl{.type = 0, .tag = 1, .field_types = {0}}
    , zero{.ctor = 0, .fields = {}}
    , one{.ctor = 1, .fields = {&zero}}
    , two{.ctor = 1, .fields = {&one}}
    , falsity{.ctor = 2, .fields = {}}
    , input{feature::var{.index = 0}}
    , predecessor{feature::var{.index = 1}}
    , second_predecessor{feature::var{.index = 2}}
    , false_output{feature::constant{.literal = &falsity}}
    , empty{model_node::empty{}}
    , false_leaf{model_node::leaf{.output = &false_output}}
    , variable_leaf{model_node::leaf{.output = &second_predecessor}}
    , inner{model_node::match{.scrutinee = &predecessor, .branches = {&empty, &variable_leaf}}}
    , outer{model_node::match{.scrutinee = &input, .branches = {&false_leaf, &inner}}}
    , evaluate_feature()
    , get_ctor_decl() {}

}

TEST_F(ModelEvaluatorTest, ALeafGivesItsOutputEvaluatedOnTheInputs) {
    EXPECT_CALL(evaluate_feature, evaluate(&false_output, std::vector<const value*>{&zero})).WillOnce(Return(&falsity));
    EXPECT_EQ(evaluator.evaluate(&false_leaf, {&zero}), std::optional<const value*>(&falsity));
}

TEST_F(ModelEvaluatorTest, EmptyGivesNothing) {
    EXPECT_FALSE(evaluator.evaluate(&empty, {&zero}).has_value());
}

TEST_F(ModelEvaluatorTest, AMatchDescendsIntoTheMatchedConstructorsBranch) {
    ON_CALL(evaluate_feature, evaluate(&input, _)).WillByDefault(Return(&zero));
    EXPECT_EQ(evaluator.evaluate(&outer, {&zero}), std::optional<const value*>(&falsity));
}

TEST_F(ModelEvaluatorTest, EachMatchBindsItsConstructorsFieldsBeforeDescending) {
    EXPECT_CALL(evaluate_feature, evaluate(&input, std::vector<const value*>{&two})).WillOnce(Return(&two));
    EXPECT_CALL(evaluate_feature, evaluate(&predecessor, std::vector<const value*>{&two, &one}))
        .WillOnce(Return(&one));
    EXPECT_CALL(evaluate_feature, evaluate(&second_predecessor, std::vector<const value*>{&two, &one, &zero}))
        .WillOnce(Return(&zero));
    EXPECT_EQ(evaluator.evaluate(&outer, {&two}), std::optional<const value*>(&zero));
}

TEST_F(ModelEvaluatorTest, AnEmptyBranchGivesNothing) {
    ON_CALL(evaluate_feature, evaluate(&input, std::vector<const value*>{&one})).WillByDefault(Return(&one));
    ON_CALL(evaluate_feature, evaluate(&predecessor, std::vector<const value*>{&one, &zero}))
        .WillByDefault(Return(&zero));
    EXPECT_FALSE(evaluator.evaluate(&outer, {&one}).has_value());
}
