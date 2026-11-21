#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_recency_measurer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindFeaturePosition {
    MOCK_METHOD(std::optional<size_t>, find_feature_position, (size_t), (const));
};

struct MockMeasureFeatureRecency {
    MOCK_METHOD(size_t, measure, (size_t), (const));
};

// Operands 0 and 1 are constants; operand 2 is the feature at position 0 and operand 3 the one at position 1.
struct OperandRecencyMeasurerTest : public ::testing::Test {
    NiceMock<MockFindFeaturePosition> find_feature_position;
    NiceMock<MockMeasureFeatureRecency> measure_feature_recency;
    operand_recency_measurer<NiceMock<MockFindFeaturePosition>, NiceMock<MockMeasureFeatureRecency>> measurer{
        find_feature_position, measure_feature_recency};

    void SetUp() override {
        ON_CALL(find_feature_position, find_feature_position(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(find_feature_position, find_feature_position(2)).WillByDefault(Return(0));
        ON_CALL(find_feature_position, find_feature_position(3)).WillByDefault(Return(1));
        ON_CALL(measure_feature_recency, measure(0)).WillByDefault(Return(0));
        ON_CALL(measure_feature_recency, measure(1)).WillByDefault(Return(5));
    }
};

}

TEST_F(OperandRecencyMeasurerTest, AConstantIsZero) {
    EXPECT_EQ(measurer.measure(0), 0u);
    EXPECT_EQ(measurer.measure(1), 0u);
}

TEST_F(OperandRecencyMeasurerTest, AFeatureIsAsRecentAsItsFeature) {
    EXPECT_EQ(measurer.measure(2), 0u);
    EXPECT_EQ(measurer.measure(3), 5u);
}

TEST_F(OperandRecencyMeasurerTest, AFeatureIsMeasuredAtItsPosition) {
    EXPECT_CALL(measure_feature_recency, measure(_)).Times(AnyNumber());
    EXPECT_CALL(measure_feature_recency, measure(1)).WillOnce(Return(8));
    EXPECT_EQ(measurer.measure(3), 8u);
}

TEST_F(OperandRecencyMeasurerTest, AConstantsRecencyIsNotMeasured) {
    EXPECT_CALL(measure_feature_recency, measure(_)).Times(0);
    measurer.measure(1);
}
