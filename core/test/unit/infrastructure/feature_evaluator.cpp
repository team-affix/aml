#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_evaluator.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace {

struct MockCallPrimitive {
    MOCK_METHOD(const value*, call, (prim_id, const std::vector<const value*>&), (const));
};

struct FeatureEvaluatorTest : public ::testing::Test {
    FeatureEvaluatorTest();
    value falsity;
    value truth;
    feature second_var;
    feature true_constant;
    NiceMock<MockCallPrimitive> call_primitive;
    feature_evaluator<NiceMock<MockCallPrimitive>> evaluator{call_primitive};
};

FeatureEvaluatorTest::FeatureEvaluatorTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , second_var{feature::var{.index = 1}}
    , true_constant{feature::constant{.literal = &truth}}
    , call_primitive() {}

}

TEST_F(FeatureEvaluatorTest, AVariableReadsItsValueFromScope) {
    EXPECT_EQ(evaluator.evaluate(second_var, {&truth, &falsity}), &falsity);
}

TEST_F(FeatureEvaluatorTest, AConstantGivesItsLiteral) {
    EXPECT_EQ(evaluator.evaluate(true_constant, {}), &truth);
}

TEST_F(FeatureEvaluatorTest, AnApplicationAppliesItsPrimitiveToItsEvaluatedArguments) {
    const feature inner{feature::app{.prim = 0, .args = {second_var}}};
    const feature outer{feature::app{.prim = 1, .args = {inner, true_constant}}};
    EXPECT_CALL(call_primitive, call(0, std::vector<const value*>{&falsity})).WillOnce(Return(&truth));
    EXPECT_CALL(call_primitive, call(1, std::vector<const value*>{&truth, &truth})).WillOnce(Return(&falsity));
    EXPECT_EQ(evaluator.evaluate(outer, {&truth, &falsity}), &falsity);
}

TEST_F(FeatureEvaluatorTest, AVariableReadsOnlyItsOwnPositionInScope) {
    const feature first_var{feature::var{.index = 0}};
    EXPECT_EQ(evaluator.evaluate(first_var, {&truth, &falsity}), &truth);
}

TEST_F(FeatureEvaluatorTest, AConstantIgnoresTheScope) {
    EXPECT_CALL(call_primitive, call(_, _)).Times(0);
    EXPECT_EQ(evaluator.evaluate(true_constant, {&falsity, &falsity}), &truth);
}

TEST_F(FeatureEvaluatorTest, ArgumentsAreEvaluatedInOrder) {
    const feature first_var{feature::var{.index = 0}};
    const feature application{feature::app{.prim = 2, .args = {first_var, second_var, true_constant}}};
    EXPECT_CALL(call_primitive, call(2, std::vector<const value*>{&falsity, &truth, &truth}))
        .WillOnce(Return(&truth));
    EXPECT_EQ(evaluator.evaluate(application, {&falsity, &truth}), &truth);
}
