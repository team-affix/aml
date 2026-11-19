#include <limits>
#include <optional>
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
    : first{model_node::leaf{.output = feature{feature::var{.index = 0}}}}
    , second{model_node::leaf{.output = feature{feature::var{.index = 1}}}}
    , tracker() {}

}

TEST_F(BestModelTrackerTest, StartsWithNoModel) {
    EXPECT_FALSE(tracker.best_model().has_value());
    EXPECT_EQ(tracker.best_cost(), std::numeric_limits<size_t>::max());
}

TEST_F(BestModelTrackerTest, ACheaperModelIsRecorded) {
    tracker.try_record(first, 9);
    tracker.try_record(second, 4);
    EXPECT_EQ(tracker.best_model(), std::optional<model_node>(second));
    EXPECT_EQ(tracker.best_cost(), 4u);
}

TEST_F(BestModelTrackerTest, AModelNoCheaperThanTheBestIsIgnored) {
    tracker.try_record(first, 4);
    tracker.try_record(second, 4);
    EXPECT_EQ(tracker.best_model(), std::optional<model_node>(first));
}

TEST_F(BestModelTrackerTest, TheFirstRecordAlwaysWins) {
    tracker.try_record(first, 1000);
    EXPECT_EQ(tracker.best_model(), std::optional<model_node>(first));
    EXPECT_EQ(tracker.best_cost(), 1000u);
}

TEST_F(BestModelTrackerTest, ACostlierModelLaterIsIgnored) {
    tracker.try_record(first, 3);
    tracker.try_record(second, 8);
    EXPECT_EQ(tracker.best_model(), std::optional<model_node>(first));
    EXPECT_EQ(tracker.best_cost(), 3u);
}

TEST_F(BestModelTrackerTest, AFreeModelIsRecorded) {
    tracker.try_record(first, 0);
    EXPECT_EQ(tracker.best_cost(), 0u);
}

TEST_F(BestModelTrackerTest, TheRecordedModelIsACopyThatOutlivesTheOriginal) {
    {
        const model_node recorded{model_node::match{
            .scrutinee = feature{feature::var{.index = 0}}, .branches = {first, model_node{model_node::empty{}}}}};
        tracker.try_record(recorded, 5);
    }
    const model_node expected{model_node::match{
        .scrutinee = feature{feature::var{.index = 0}}, .branches = {first, model_node{model_node::empty{}}}}};
    EXPECT_EQ(tracker.best_model(), std::optional<model_node>(expected));
}
