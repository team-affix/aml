#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/argument_option_finder.hpp"

using ::testing::AnyNumber;
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

struct MockDetectWitness {
    MOCK_METHOD(bool, detect, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&), (const));
};

candidate typed(type_id type) {
    return candidate{.expression = nullptr, .cost = 1, .observed = typed_column{.type = type, .cells = {}}};
}

constant_operand constant_of(type_id type) {
    return constant_operand{.expression = nullptr, .literal = nullptr, .type = type};
}

// Constants: [0] type 0, [1] type 1. Features: [2] type 0, [3] type 0.
struct ArgumentOptionFinderTest : public ::testing::Test {
    ArgumentOptionFinderTest();
    std::vector<constant_operand> constants;
    std::vector<candidate> features;
    NiceMock<MockGetConstants> get_constants;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockDetectWitness> detect_witness;
    argument_option_finder<
        NiceMock<MockGetConstants>, NiceMock<MockGetFeatureCandidates>, NiceMock<MockDetectWitness>>
        finder{get_constants, get_feature_candidates, detect_witness};

    void SetUp() override {
        ON_CALL(get_constants, constants()).WillByDefault(ReturnRef(constants));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(detect_witness, detect(_, _, _)).WillByDefault(Return(true));
    }
};

ArgumentOptionFinderTest::ArgumentOptionFinderTest()
    : constants{constant_of(0), constant_of(1)}
    , features{typed(0), typed(0)}
    , get_constants()
    , get_feature_candidates()
    , detect_witness() {}

}

TEST_F(ArgumentOptionFinderTest, OperandsOfTheNextArgumentsTypeAreOptionsConstantsFirst) {
    EXPECT_EQ(finder.find(0, {1, 0}, {1}), (std::vector<size_t>{0, 2, 3}));
}

TEST_F(ArgumentOptionFinderTest, AnOperandThatLeavesNoWitnessIsNotAnOption) {
    ON_CALL(detect_witness, detect(0, _, std::vector<size_t>{1, 2})).WillByDefault(Return(false));
    EXPECT_EQ(finder.find(0, {1, 0}, {1}), (std::vector<size_t>{0, 3}));
}

TEST_F(ArgumentOptionFinderTest, EachOperandIsTriedAsThePrefixExtended) {
    EXPECT_CALL(detect_witness, detect(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(detect_witness, detect(5, std::vector<type_id>{1, 0}, std::vector<size_t>{1, 3}))
        .WillOnce(Return(true));
    finder.find(5, {1, 0}, {1});
}

TEST_F(ArgumentOptionFinderTest, TheFirstArgumentIsChosenFromAnEmptyPrefix) {
    EXPECT_EQ(finder.find(0, {1, 0}, {}), (std::vector<size_t>{1}));
}
