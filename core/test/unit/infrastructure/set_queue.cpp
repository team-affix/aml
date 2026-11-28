#include <optional>
#include <gtest/gtest.h>
#include "infrastructure/set_queue.hpp"

using namespace AML;

namespace {

struct SetQueueTest : public ::testing::Test {
    SetQueueTest();
    set_queue queue;
};

SetQueueTest::SetQueueTest()
    : queue(3) {}

}

TEST_F(SetQueueTest, AnEmptyQueuePopsNothing) {
    EXPECT_EQ(queue.pop(), std::nullopt);
}

TEST_F(SetQueueTest, TheCheapestSetPopsFirst) {
    queue.push(queued_set{.members = {1}, .cost = 9});
    queue.push(queued_set{.members = {2}, .cost = 4});
    EXPECT_EQ(queue.pop(), std::optional<queued_set>(queued_set{.members = {2}, .cost = 4}));
    EXPECT_EQ(queue.pop(), std::optional<queued_set>(queued_set{.members = {1}, .cost = 9}));
    EXPECT_EQ(queue.pop(), std::nullopt);
}

TEST_F(SetQueueTest, SetsOfEqualCostPopInTheOrderTheyCame) {
    queue.push(queued_set{.members = {1}, .cost = 4});
    queue.push(queued_set{.members = {2}, .cost = 4});
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{1}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{2}));
}

TEST_F(SetQueueTest, PastCapacityTheCostliestSetIsDropped) {
    queue.push(queued_set{.members = {1}, .cost = 5});
    queue.push(queued_set{.members = {2}, .cost = 9});
    queue.push(queued_set{.members = {3}, .cost = 2});
    queue.push(queued_set{.members = {4}, .cost = 7});
    EXPECT_EQ(queue.size(), 3u);
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{3}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{1}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{4}));
}

TEST_F(SetQueueTest, ASetCostlierThanAFullQueueIsDroppedItself) {
    queue.push(queued_set{.members = {1}, .cost = 1});
    queue.push(queued_set{.members = {2}, .cost = 2});
    queue.push(queued_set{.members = {3}, .cost = 3});
    queue.push(queued_set{.members = {4}, .cost = 8});
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{1}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{2}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{3}));
    EXPECT_EQ(queue.pop(), std::nullopt);
}

TEST_F(SetQueueTest, AmongEqualCostliestTheLatestIsDropped) {
    queue.push(queued_set{.members = {1}, .cost = 1});
    queue.push(queued_set{.members = {2}, .cost = 6});
    queue.push(queued_set{.members = {3}, .cost = 6});
    queue.push(queued_set{.members = {4}, .cost = 6});
    queue.pop();
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{2}));
    EXPECT_EQ(queue.pop().value().members, (std::vector<feature_id>{3}));
}

TEST_F(SetQueueTest, TheEmptySetCanBeQueued) {
    queue.push(queued_set{.members = {}, .cost = 1});
    EXPECT_EQ(queue.pop(), std::optional<queued_set>(queued_set{.members = {}, .cost = 1}));
}
