#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_quadrant_measurer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetFeatureOperand {
    MOCK_METHOD(size_t, feature_operand, (size_t), (const));
};

struct MockMeasureOperandQuadrant {
    MOCK_METHOD(size_t, measure, (size_t), (const));
};

struct FeatureQuadrantMeasurerTest : public ::testing::Test {
    NiceMock<MockGetFeatureOperand> get_feature_operand;
    NiceMock<MockMeasureOperandQuadrant> measure_operand_quadrant;
    feature_quadrant_measurer<NiceMock<MockGetFeatureOperand>, NiceMock<MockMeasureOperandQuadrant>> measurer{
        get_feature_operand, measure_operand_quadrant};
};

}

TEST_F(FeatureQuadrantMeasurerTest, AFeatureIsInItsOperandsQuadrant) {
    ON_CALL(get_feature_operand, feature_operand(1)).WillByDefault(Return(4));
    ON_CALL(measure_operand_quadrant, measure(4)).WillByDefault(Return(3));
    EXPECT_EQ(measurer.measure(1), 3u);
}

TEST_F(FeatureQuadrantMeasurerTest, TheOperandMeasuredIsTheFeaturesOwn) {
    ON_CALL(get_feature_operand, feature_operand(0)).WillByDefault(Return(2));
    EXPECT_CALL(measure_operand_quadrant, measure(_)).Times(AnyNumber());
    EXPECT_CALL(measure_operand_quadrant, measure(2)).WillOnce(Return(1));
    EXPECT_EQ(measurer.measure(0), 1u);
}
