#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/binary_option_chooser.hpp"

using namespace AML;

using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRoundRobin;
using ::testing::_;

namespace {

struct MockChooseHalf {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct BinaryOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseHalf> choose_half;
    binary_option_chooser<NiceMock<MockChooseHalf>> chooser{choose_half};
};

}

TEST_F(BinaryOptionChooserTest, ASingleOptionIsChosenWithoutAsking) {
    EXPECT_CALL(choose_half, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose(1), 0u);
}

TEST_F(BinaryOptionChooserTest, TwoOptionsAreOneTwoWayChoice) {
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(2), 1u);
}

TEST_F(BinaryOptionChooserTest, EveryQuestionIsTwoWay) {
    EXPECT_CALL(choose_half, choose(_)).Times(0);
    EXPECT_CALL(choose_half, choose(2)).Times(AnyNumber()).WillRepeatedly(Return(1));
    chooser.choose(13);
    chooser.choose(4);
    chooser.choose(100);
}

TEST_F(BinaryOptionChooserTest, EachAnswerKeepsTheChosenHalf) {
    InSequence order;
    // [0, 13) → upper [7, 13) → lower [7, 10) → upper [9, 10).
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(13), 9u);
}

TEST_F(BinaryOptionChooserTest, AnOddRangesLowerHalfTakesTheExtraIndex) {
    InSequence order;
    // [0, 3) → lower [0, 2) → upper [1, 2).
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_half, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(3), 1u);
}

TEST_F(BinaryOptionChooserTest, AlwaysKeepingTheLowerHalfGivesTheFirstOption) {
    // [0, 13) → [0, 7) → [0, 4) → [0, 2) → [0, 1).
    EXPECT_CALL(choose_half, choose(2)).Times(4).WillRepeatedly(Return(0));
    EXPECT_EQ(chooser.choose(13), 0u);
}

TEST_F(BinaryOptionChooserTest, AlwaysKeepingTheUpperHalfGivesTheLastOption) {
    // [0, 13) → [7, 13) → [10, 13) → [12, 13).
    EXPECT_CALL(choose_half, choose(2)).Times(3).WillRepeatedly(Return(1));
    EXPECT_EQ(chooser.choose(13), 12u);
}

TEST_F(BinaryOptionChooserTest, EveryOptionIsReachableAndNoneLiesOutside) {
    for (size_t option_count = 1; option_count <= 17; ++option_count) {
        std::set<size_t> reached;
        for (unsigned pattern = 0; pattern < 32; ++pattern) {
            std::vector<size_t> answers;
            for (unsigned bit = 0; bit < 5; ++bit)
                answers.push_back((pattern >> bit) & 1);
            ON_CALL(choose_half, choose(2)).WillByDefault(ReturnRoundRobin(answers));
            const size_t chosen = chooser.choose(option_count);
            EXPECT_LT(chosen, option_count);
            reached.insert(chosen);
        }
        EXPECT_EQ(reached.size(), option_count);
    }
}

TEST_F(BinaryOptionChooserTest, EachChoiceStartsFromTheFullRange) {
    EXPECT_CALL(choose_half, choose(2)).WillRepeatedly(Return(1));
    EXPECT_EQ(chooser.choose(4), 3u);
    EXPECT_EQ(chooser.choose(4), 3u);
    EXPECT_EQ(chooser.choose(2), 1u);
}
