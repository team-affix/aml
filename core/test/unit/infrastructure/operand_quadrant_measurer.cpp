#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_quadrant_measurer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetOperandExpression {
    MOCK_METHOD(const feature&, expression, (size_t), (const));
};

struct MockCheckUsed {
    MOCK_METHOD(bool, is_used, (size_t), (const));
};

// Operand 0 is a constant, 1 a variable, 2 an application.
struct OperandQuadrantMeasurerTest : public ::testing::Test {
    OperandQuadrantMeasurerTest();
    feature constant;
    feature variable;
    feature application;
    NiceMock<MockGetOperandExpression> get_operand_expression;
    NiceMock<MockCheckUsed> check_used;
    operand_quadrant_measurer<NiceMock<MockGetOperandExpression>, NiceMock<MockCheckUsed>> measurer{
        get_operand_expression, check_used};

    void SetUp() override {
        ON_CALL(get_operand_expression, expression(0)).WillByDefault(ReturnRef(constant));
        ON_CALL(get_operand_expression, expression(1)).WillByDefault(ReturnRef(variable));
        ON_CALL(get_operand_expression, expression(2)).WillByDefault(ReturnRef(application));
        ON_CALL(check_used, is_used(_)).WillByDefault(Return(false));
    }
};

OperandQuadrantMeasurerTest::OperandQuadrantMeasurerTest()
    : constant{feature::constant{.literal = value{.ctor = 1, .args = {}}}}
    , variable{feature::var{.index = 0}}
    , application{feature::app{.prim = 0, .args = {feature{feature::var{.index = 0}}}}}
    , get_operand_expression()
    , check_used() {}

}

TEST_F(OperandQuadrantMeasurerTest, AnUnusedApplicationIsMadeAndFresh) {
    EXPECT_EQ(measurer.measure(2), k_fresh_quadrant_bit + k_made_quadrant_bit);
}

TEST_F(OperandQuadrantMeasurerTest, AnUnusedVariableOrConstantIsGivenAndFresh) {
    EXPECT_EQ(measurer.measure(1), k_fresh_quadrant_bit);
    EXPECT_EQ(measurer.measure(0), k_fresh_quadrant_bit);
}

TEST_F(OperandQuadrantMeasurerTest, AUsedApplicationIsMadeAndConsumed) {
    ON_CALL(check_used, is_used(2)).WillByDefault(Return(true));
    EXPECT_EQ(measurer.measure(2), k_made_quadrant_bit);
}

TEST_F(OperandQuadrantMeasurerTest, AUsedVariableIsGivenAndConsumed) {
    ON_CALL(check_used, is_used(1)).WillByDefault(Return(true));
    EXPECT_EQ(measurer.measure(1), 0u);
}

TEST_F(OperandQuadrantMeasurerTest, UseIsCheckedForTheOperandMeasured) {
    EXPECT_CALL(check_used, is_used(_)).Times(AnyNumber());
    EXPECT_CALL(check_used, is_used(2)).WillOnce(Return(true));
    measurer.measure(2);
}
