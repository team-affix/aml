#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/alternating_set_popper.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPopSet {
    MOCK_METHOD(std::optional<queued_set>, pop, ());
};

struct AlternatingSetPopperTest : public ::testing::Test {
    AlternatingSetPopperTest();
    queued_set first;
    queued_set second;
    NiceMock<MockPopSet> pop_set0;
    NiceMock<MockPopSet> pop_set1;
    alternating_set_popper<NiceMock<MockPopSet>, NiceMock<MockPopSet>> popper{pop_set0, pop_set1};

    void SetUp() override {
        ON_CALL(pop_set0, pop()).WillByDefault(Return(first));
        ON_CALL(pop_set1, pop()).WillByDefault(Return(second));
    }
};

AlternatingSetPopperTest::AlternatingSetPopperTest()
    : first{.members = {1}, .cost = 3}
    , second{.members = {2}, .cost = 4}
    , pop_set0()
    , pop_set1() {}

}

TEST_F(AlternatingSetPopperTest, TheSourcesTakeTurnsTheFirstFirst) {
    EXPECT_EQ(popper.pop(), first);
    EXPECT_EQ(popper.pop(), second);
    EXPECT_EQ(popper.pop(), first);
    EXPECT_EQ(popper.pop(), second);
}

TEST_F(AlternatingSetPopperTest, AnEmptyFirstSourcePassesItsTurn) {
    ON_CALL(pop_set0, pop()).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(popper.pop(), second);
    EXPECT_EQ(popper.pop(), second);
}

TEST_F(AlternatingSetPopperTest, AnEmptySecondSourcePassesItsTurn) {
    ON_CALL(pop_set1, pop()).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(popper.pop(), first);
    EXPECT_EQ(popper.pop(), first);
}

TEST_F(AlternatingSetPopperTest, BothEmptyGiveNothing) {
    ON_CALL(pop_set0, pop()).WillByDefault(Return(std::nullopt));
    ON_CALL(pop_set1, pop()).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(popper.pop().has_value());
    EXPECT_FALSE(popper.pop().has_value());
}

TEST_F(AlternatingSetPopperTest, APassedTurnStillMovesTheTurnOn) {
    EXPECT_CALL(pop_set0, pop()).WillOnce(Return(std::nullopt)).WillRepeatedly(Return(first));
    EXPECT_EQ(popper.pop(), second);
    EXPECT_EQ(popper.pop(), second);
    EXPECT_EQ(popper.pop(), first);
}
