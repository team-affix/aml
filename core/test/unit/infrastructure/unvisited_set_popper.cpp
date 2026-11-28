#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/unvisited_set_popper.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPopSet {
    MOCK_METHOD(std::optional<queued_set>, pop, ());
};

struct MockMeasureSetKey {
    MOCK_METHOD(uint64_t, measure, (const std::vector<feature_id>&), (const));
};

struct MockTryVisit {
    MOCK_METHOD(bool, try_visit, (uint64_t, size_t));
};

// The set {1} has key 10, {2} has key 20.
struct UnvisitedSetPopperTest : public ::testing::Test {
    UnvisitedSetPopperTest();
    queued_set first;
    queued_set second;
    NiceMock<MockPopSet> pop_set;
    NiceMock<MockMeasureSetKey> measure_set_key;
    NiceMock<MockTryVisit> try_visit;
    unvisited_set_popper<NiceMock<MockPopSet>, NiceMock<MockMeasureSetKey>, NiceMock<MockTryVisit>> popper{
        pop_set, measure_set_key, try_visit};

    void SetUp() override {
        ON_CALL(measure_set_key, measure(std::vector<feature_id>{1})).WillByDefault(Return(10));
        ON_CALL(measure_set_key, measure(std::vector<feature_id>{2})).WillByDefault(Return(20));
        ON_CALL(try_visit, try_visit(_, _)).WillByDefault(Return(true));
    }
};

UnvisitedSetPopperTest::UnvisitedSetPopperTest()
    : first{.members = {1}, .cost = 3}
    , second{.members = {2}, .cost = 4}
    , pop_set()
    , measure_set_key()
    , try_visit() {}

}

TEST_F(UnvisitedSetPopperTest, AnUnvisitedSetIsReturnedAndRecordedByItsKeyAndCost) {
    ON_CALL(pop_set, pop()).WillByDefault(Return(first));
    EXPECT_CALL(try_visit, try_visit(10, 3)).WillOnce(Return(true));
    EXPECT_EQ(popper.pop(), first);
}

TEST_F(UnvisitedSetPopperTest, VisitedSetsAreDiscardedUntilAnUnvisitedOne) {
    EXPECT_CALL(pop_set, pop()).WillOnce(Return(first)).WillOnce(Return(first)).WillOnce(Return(second));
    ON_CALL(try_visit, try_visit(10, 3)).WillByDefault(Return(false));
    EXPECT_EQ(popper.pop(), second);
}

TEST_F(UnvisitedSetPopperTest, AnEmptySourceGivesNothing) {
    ON_CALL(pop_set, pop()).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(popper.pop().has_value());
}

TEST_F(UnvisitedSetPopperTest, ASourceEmptiedByDiscardsGivesNothing) {
    EXPECT_CALL(pop_set, pop()).WillOnce(Return(first)).WillOnce(Return(std::nullopt));
    ON_CALL(try_visit, try_visit(10, 3)).WillByDefault(Return(false));
    EXPECT_FALSE(popper.pop().has_value());
}
