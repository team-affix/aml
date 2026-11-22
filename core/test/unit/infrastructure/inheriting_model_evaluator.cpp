#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/inheriting_model_evaluator.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockEvaluateFeature {
    MOCK_METHOD(value, evaluate, (const feature&, const std::vector<value>&), (const));
};

struct MockFindScrutineePieces {
    MOCK_METHOD(std::vector<feature>, find, (const feature&), (const));
};

// One input x0. The scrutinee `sum` has pieces `sum` and `twice`; matched as suc(arg), branch 1 binds x1 = arg, then
// x2 = sum's value and x3 = twice's value.
struct InheritingModelEvaluatorTest : public ::testing::Test {
    InheritingModelEvaluatorTest();
    static value leaf(ctor_id ctor);
    feature x0;
    feature twice;
    feature sum;
    feature output;
    value input;
    value argument;
    value matched;
    value sum_value;
    value twice_value;
    value answer;
    model_node model;
    NiceMock<MockEvaluateFeature> evaluate_feature;
    NiceMock<MockFindScrutineePieces> find_scrutinee_pieces;
    inheriting_model_evaluator<NiceMock<MockEvaluateFeature>, NiceMock<MockFindScrutineePieces>> evaluator{
        evaluate_feature, find_scrutinee_pieces};

    void SetUp() override {
        ON_CALL(find_scrutinee_pieces, find(sum)).WillByDefault(Return(std::vector<feature>{sum, twice}));
        ON_CALL(evaluate_feature, evaluate(sum, _)).WillByDefault(Return(matched));
        ON_CALL(evaluate_feature, evaluate(twice, _)).WillByDefault(Return(twice_value));
        ON_CALL(evaluate_feature, evaluate(output, _)).WillByDefault(Return(answer));
    }
};

InheritingModelEvaluatorTest::InheritingModelEvaluatorTest()
    : x0{feature::var{.index = 0}}
    , twice{feature::app{.prim = 1, .args = {x0}}}
    , sum{feature::app{.prim = 0, .args = {twice, x0}}}
    , output{feature::var{.index = 3}}
    , input(leaf(5))
    , argument(leaf(6))
    , matched{.ctor = 1, .args = {argument}}
    , sum_value(matched)
    , twice_value(leaf(7))
    , answer(leaf(8))
    , model{model_node::match{
          .scrutinee = sum,
          .branches = {model_node{model_node::empty{}}, model_node{model_node::leaf{.output = output}}}}}
    , evaluate_feature()
    , find_scrutinee_pieces() {}

value InheritingModelEvaluatorTest::leaf(ctor_id ctor) {
    return value{.ctor = ctor, .args = {}};
}

}

TEST_F(InheritingModelEvaluatorTest, ABranchSeesItsArgumentsThenThePiecesValues) {
    EXPECT_CALL(evaluate_feature, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature, evaluate(output, (std::vector<value>{input, argument, sum_value, twice_value})))
        .WillOnce(Return(answer));
    EXPECT_EQ(evaluator.evaluate(model, {input}), std::optional<value>(answer));
}

TEST_F(InheritingModelEvaluatorTest, ThePiecesAreEvaluatedInTheMatchsScope) {
    EXPECT_CALL(evaluate_feature, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature, evaluate(twice, std::vector<value>{input})).WillOnce(Return(twice_value));
    evaluator.evaluate(model, {input});
}

TEST_F(InheritingModelEvaluatorTest, AVariableScrutineePassesOnlyItsArguments) {
    const model_node by_variable{model_node::match{
        .scrutinee = x0,
        .branches = {model_node{model_node::empty{}}, model_node{model_node::leaf{.output = output}}}}};
    ON_CALL(find_scrutinee_pieces, find(x0)).WillByDefault(Return(std::vector<feature>{}));
    ON_CALL(evaluate_feature, evaluate(x0, _)).WillByDefault(Return(matched));
    EXPECT_CALL(evaluate_feature, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature, evaluate(output, (std::vector<value>{input, argument}))).WillOnce(Return(answer));
    evaluator.evaluate(by_variable, {input});
}

TEST_F(InheritingModelEvaluatorTest, ALeafGivesItsOutput) {
    const model_node single{model_node::leaf{.output = output}};
    EXPECT_EQ(evaluator.evaluate(single, {input}), std::optional<value>(answer));
}

TEST_F(InheritingModelEvaluatorTest, AnEmptyBranchGivesNothing) {
    ON_CALL(evaluate_feature, evaluate(sum, _)).WillByDefault(Return(leaf(0)));
    EXPECT_EQ(evaluator.evaluate(model, {input}), std::nullopt);
}

TEST_F(InheritingModelEvaluatorTest, NestedMatchesStackTheirVariables) {
    const model_node nested{model_node::match{
        .scrutinee = sum,
        .branches = {model_node{model_node::empty{}}, model}}};
    EXPECT_CALL(evaluate_feature, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature,
                evaluate(output, (std::vector<value>{input, argument, sum_value, twice_value, argument, sum_value,
                                                      twice_value})))
        .WillOnce(Return(answer));
    evaluator.evaluate(nested, {input});
}
