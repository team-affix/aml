#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/argument_option_finder.hpp"

using namespace AML;

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockCountOperandsOfType {
    MOCK_METHOD(size_t, count_of_type, (type_id), (const));
};

struct MockGetOperandOfType {
    MOCK_METHOD(size_t, operand_of_type, (type_id, size_t), (const));
};

struct MockDetectWitness {
    MOCK_METHOD(bool, detect, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&), (const));
};

// Type 0's operands are 0, 2 and 3; type 1's is 1.
struct ArgumentOptionFinderTest : public ::testing::Test {
    NiceMock<MockCountOperandsOfType> count_operands_of_type;
    NiceMock<MockGetOperandOfType> get_operand_of_type;
    NiceMock<MockDetectWitness> detect_witness;
    argument_option_finder<
        NiceMock<MockCountOperandsOfType>, NiceMock<MockGetOperandOfType>, NiceMock<MockDetectWitness>>
        finder{count_operands_of_type, get_operand_of_type, detect_witness};

    void SetUp() override {
        ON_CALL(count_operands_of_type, count_of_type(0)).WillByDefault(Return(3));
        ON_CALL(count_operands_of_type, count_of_type(1)).WillByDefault(Return(1));
        ON_CALL(get_operand_of_type, operand_of_type(0, 0)).WillByDefault(Return(0));
        ON_CALL(get_operand_of_type, operand_of_type(0, 1)).WillByDefault(Return(2));
        ON_CALL(get_operand_of_type, operand_of_type(0, 2)).WillByDefault(Return(3));
        ON_CALL(get_operand_of_type, operand_of_type(1, 0)).WillByDefault(Return(1));
        ON_CALL(detect_witness, detect(_, _, _)).WillByDefault(Return(true));
    }
};

}

TEST_F(ArgumentOptionFinderTest, TheNextArgumentTypesOperandsAreOptionsInOperandOrder) {
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

TEST_F(ArgumentOptionFinderTest, OnlyOperandsOfTheNextTypeAreLookedAt) {
    EXPECT_CALL(get_operand_of_type, operand_of_type(_, _)).Times(AnyNumber());
    EXPECT_CALL(get_operand_of_type, operand_of_type(0, _)).Times(0);
    finder.find(0, {1, 0}, {});
}

TEST_F(ArgumentOptionFinderTest, ATypeWithNoOperandsGivesNoOptions) {
    EXPECT_TRUE(finder.find(0, {6}, {}).empty());
}

TEST_F(ArgumentOptionFinderTest, NoOptionsWhenNoOperandLeavesAWitness) {
    ON_CALL(detect_witness, detect(_, _, _)).WillByDefault(Return(false));
    EXPECT_TRUE(finder.find(0, {0, 0}, {}).empty());
}
