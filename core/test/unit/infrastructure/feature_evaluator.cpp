#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_evaluator.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockApplyPrimitive {
    MOCK_METHOD(const value*, apply, (prim_id, const std::vector<const value*>&), (const));
};

struct FeatureEvaluatorTest : public ::testing::Test {
    FeatureEvaluatorTest();
    value falsity;
    value truth;
    feature second_var;
    feature true_constant;
    NiceMock<MockApplyPrimitive> apply_primitive;
    feature_evaluator<NiceMock<MockApplyPrimitive>> evaluator{apply_primitive};
};

FeatureEvaluatorTest::FeatureEvaluatorTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , second_var{feature::var{.index = 1}}
    , true_constant{feature::constant{.literal = &truth}}
    , apply_primitive() {}

}

TEST_F(FeatureEvaluatorTest, AVariableReadsItsValueFromScope) {
    EXPECT_EQ(evaluator.evaluate(&second_var, {&truth, &falsity}), &falsity);
}

TEST_F(FeatureEvaluatorTest, AConstantGivesItsLiteral) {
    EXPECT_EQ(evaluator.evaluate(&true_constant, {}), &truth);
}

TEST_F(FeatureEvaluatorTest, AnApplicationAppliesItsPrimitiveToItsEvaluatedArguments) {
    const feature inner{feature::app{.prim = 0, .args = {&second_var}}};
    const feature outer{feature::app{.prim = 1, .args = {&inner, &true_constant}}};
    EXPECT_CALL(apply_primitive, apply(0, std::vector<const value*>{&falsity})).WillOnce(Return(&truth));
    EXPECT_CALL(apply_primitive, apply(1, std::vector<const value*>{&truth, &truth})).WillOnce(Return(&falsity));
    EXPECT_EQ(evaluator.evaluate(&outer, {&truth, &falsity}), &falsity);
}
