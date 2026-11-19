#include <limits>
#include <gtest/gtest.h>
#include "infrastructure/best_model_tracker.hpp"

namespace {

struct BestModelTrackerTest : public ::testing::Test {
    BestModelTrackerTest();
    model_node first;
    model_node second;
    best_model_tracker tracker;
};

BestModelTrackerTest::BestModelTrackerTest()
    : first{model_node::empty{}}
    , second{model_node::empty{}}
    , tracker() {}

}

TEST_F(BestModelTrackerTest, StartsWithNoModel) {
    EXPECT_EQ(tracker.best_model(), nullptr);
    EXPECT_EQ(tracker.best_cost(), std::numeric_limits<size_t>::max());
}

TEST_F(BestModelTrackerTest, ACheaperModelIsRecorded) {
    tracker.try_record(&first, 9);
    tracker.try_record(&second, 4);
    EXPECT_EQ(tracker.best_model(), &second);
    EXPECT_EQ(tracker.best_cost(), 4u);
}

TEST_F(BestModelTrackerTest, AModelNoCheaperThanTheBestIsIgnored) {
    tracker.try_record(&first, 4);
    tracker.try_record(&second, 4);
    EXPECT_EQ(tracker.best_model(), &first);
}

TEST_F(BestModelTrackerTest, TheFirstRecordAlwaysWins) {
    tracker.try_record(&first, 1000);
    EXPECT_EQ(tracker.best_model(), &first);
    EXPECT_EQ(tracker.best_cost(), 1000u);
}

TEST_F(BestModelTrackerTest, ACostlierModelLaterIsIgnored) {
    tracker.try_record(&first, 3);
    tracker.try_record(&second, 8);
    EXPECT_EQ(tracker.best_model(), &first);
    EXPECT_EQ(tracker.best_cost(), 3u);
}

TEST_F(BestModelTrackerTest, AFreeModelIsRecorded) {
    tracker.try_record(&first, 0);
    EXPECT_EQ(tracker.best_cost(), 0u);
}
