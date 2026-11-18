#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/witness_detector.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetConstants {
    MOCK_METHOD(const std::vector<constant_operand>&, constants, (), (const));
};

struct MockGetFeatureCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockCountTriedApplications {
    MOCK_METHOD(size_t, count_with_prefix, (prim_id, const std::vector<size_t>&), (const));
};

candidate typed(type_id type) {
    return candidate{.expression = nullptr, .cost = 1, .observed = typed_column{.type = type, .cells = {}}};
}

constant_operand constant_of(type_id type) {
    return constant_operand{.expression = nullptr, .literal = nullptr, .type = type};
}

// Constants: [0] type 0, [1] type 0. Features: [2] type 0, [3] type 1.
struct WitnessDetectorTest : public ::testing::Test {
    WitnessDetectorTest();
    std::vector<constant_operand> constants;
    std::vector<candidate> features;
    NiceMock<MockGetConstants> get_constants;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockCountTriedApplications> count_tried_applications;
    witness_detector<
        NiceMock<MockGetConstants>, NiceMock<MockGetFeatureCandidates>, NiceMock<MockCountTriedApplications>>
        detector{get_constants, get_feature_candidates, count_tried_applications};

    void SetUp() override {
        ON_CALL(get_constants, constants()).WillByDefault(ReturnRef(constants));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(count_tried_applications, count_with_prefix(_, _)).WillByDefault(Return(0));
    }
};

WitnessDetectorTest::WitnessDetectorTest()
    : constants{constant_of(0), constant_of(0)}
    , features{typed(0), typed(1)}
    , get_constants()
    , get_feature_candidates()
    , count_tried_applications() {}

}

TEST_F(WitnessDetectorTest, AnUntriedTupleWithAFeatureIsAWitness) {
    EXPECT_TRUE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, TuplesOfConstantsOnlyAreNotWitnesses) {
    // Type 0 has 3 operands, 2 of them constants: 9 tuples, 4 all-constant, 5 useful.
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(5));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, AWitnessRemainsWhileFewerUsefulTuplesThanThatHaveBeenTried) {
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{})).WillByDefault(Return(4));
    EXPECT_TRUE(detector.detect(0, {0, 0}, {}));
}

TEST_F(WitnessDetectorTest, APrefixHoldingAFeatureMakesEveryCompletionUseful) {
    // Prefix {2} is a feature; 3 completions of type 0, all useful.
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{2})).WillByDefault(Return(2));
    EXPECT_TRUE(detector.detect(0, {0, 0}, {2}));
    ON_CALL(count_tried_applications, count_with_prefix(0, std::vector<size_t>{2})).WillByDefault(Return(3));
    EXPECT_FALSE(detector.detect(0, {0, 0}, {2}));
}

TEST_F(WitnessDetectorTest, APrefixOfConstantsNeedsAFeatureToComplete) {
    // Prefix {0} is a constant; of 3 completions only the feature 2 is useful.
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
