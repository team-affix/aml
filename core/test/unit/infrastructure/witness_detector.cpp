#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/witness_detector.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockCheckFeature {
    MOCK_METHOD(bool, is_feature, (size_t), (const));
};

struct MockCountOperandsOfType {
    MOCK_METHOD(size_t, count_of_type, (type_id), (const));
};

struct MockCountConstantsOfType {
    MOCK_METHOD(size_t, constant_count_of_type, (type_id), (const));
};

struct MockCountTriedApplications {
    MOCK_METHOD(size_t, count_with_prefix, (prim_id, const std::vector<size_t>&), (const));
};

// Type 0 has 3 operands, 2 of them constants (operands 0 and 1); type 1 has 1 operand, a feature. Type 7 has none.
// Operands 2 and up are features.
struct WitnessDetectorTest : public ::testing::Test {
    NiceMock<MockCheckFeature> check_feature;
    NiceMock<MockCountOperandsOfType> count_operands_of_type;
    NiceMock<MockCountConstantsOfType> count_constants_of_type;
    NiceMock<MockCountTriedApplications> count_tried_applications;
    witness_detector<
        NiceMock<MockCheckFeature>, NiceMock<MockCountOperandsOfType>, NiceMock<MockCountConstantsOfType>,
        NiceMock<MockCountTriedApplications>>
        detector{check_feature, count_operands_of_type, count_constants_of_type, count_tried_applications};

    void SetUp() override {
        ON_CALL(check_feature, is_feature(_)).WillByDefault(Return(true));
        ON_CALL(check_feature, is_feature(0)).WillByDefault(Return(false));
        ON_CALL(check_feature, is_feature(1)).WillByDefault(Return(false));
        ON_CALL(count_operands_of_type, count_of_type(_)).WillByDefault(Return(0));
        ON_CALL(count_operands_of_type, count_of_type(0)).WillByDefault(Return(3));
        ON_CALL(count_operands_of_type, count_of_type(1)).WillByDefault(Return(1));
        ON_CALL(count_constants_of_type, constant_count_of_type(_)).WillByDefault(Return(0));
        ON_CALL(count_constants_of_type, constant_count_of_type(0)).WillByDefault(Return(2));
        ON_CALL(count_tried_applications, count_with_prefix(_, _)).WillByDefault(Return(0));
    }
};

}

TEST_F(WitnessDetectorTest, AnUntriedTupleWithAFeatureIsAWitness) {
    EXPECT_TRUE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, TuplesOfConstantsOnlyAreNotWitnesses) {
    // 9 tuples of type 0, 4 of them all constants: 5 useful.
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(5));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, AWitnessRemainsWhileFewerUsefulTuplesThanThatHaveBeenTried) {
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(4));
    EXPECT_TRUE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, APrefixHoldingAFeatureMakesEveryCompletionUseful) {
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{2})).WillByDefault(Return(2));
    EXPECT_TRUE(detector.detect(0, {0, 0}, {2}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{2})).WillByDefault(Return(3));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {2}));
}

TEST_F(WitnessDetectorTest, APrefixOfConstantsNeedsAFeatureToComplete) {
    EXPECT_TRUE(detector.detect(0, {0, 0}, {0}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{0})).WillByDefault(Return(1));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {0}));
}

TEST_F(WitnessDetectorTest, ACompleteTupleIsAWitnessOnlyIfItHoldsAFeatureAndIsUntried) {
    EXPECT_TRUE(detector.detect(0, {0, 1}, {0, 3}));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {0, 1}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{0, 3})).WillByDefault(Return(1));
    EXPECT_FALSE(detector.detect(0, {0, 1}, {0, 3}));
}

TEST_F(WitnessDetectorTest, AnArgumentTypeWithNoOperandsLeavesNoWitness) {
    EXPECT_FALSE(detector.detect(0, {0, 7}, {}));
}

TEST_F(WitnessDetectorTest, TriedApplicationsAreCountedForThatPrimitiveAndPrefix) {
    EXPECT_CALL(count_tried_applications, count_with_prefix(4, std::vector<size_t>{2})).WillOnce(Return(0));
    detector.detect(4, {0, 0}, {2});
}

TEST_F(WitnessDetectorTest, ANullaryPrimitiveHasNoWitnessSinceItHoldsNoFeature) {
    EXPECT_FALSE(detector.detect(0, {}, {}));
}

TEST_F(WitnessDetectorTest, AUnaryPrimitiveHasOneWitnessPerUntriedFeature) {
    EXPECT_TRUE(detector.detect(0, {1}, {}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(1));
    EXPECT_FALSE(detector.detect(0, {1}, {}));
}

TEST_F(WitnessDetectorTest, MixedArgumentTypesMultiplyTheirCounts) {
    // Type 0 then type 1: 3 * 1 = 3 tuples; the type 1 operand is a feature, so all are useful.
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(2));
    EXPECT_TRUE(detector.detect(0, {0, 1}, {}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(3));
    EXPECT_FALSE(detector.detect(0, {0, 1}, {}));
}
