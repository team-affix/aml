#include <limits>
#include <typeindex>
#include <gtest/gtest.h>
#include "infrastructure/best_table_tracker.hpp"

using namespace AML;

namespace {

struct BestTableTrackerTest : public ::testing::Test {
    BestTableTrackerTest();
    table_model model_reading(var_id input);
    table_model first;
    table_model second;
    best_table_tracker tracker;
};

BestTableTrackerTest::BestTableTrackerTest()
    : first(model_reading(0))
    , second(model_reading(1))
    , tracker() {}

table_model BestTableTrackerTest::model_reading(var_id input) {
    return table_model{
        .graph = model_graph{
            .nodes = {derivation{derivation::input{.index = input}}}, .types = {std::type_index(typeid(bool))},
            .outputs = {0}},
        .columns = {},
        .rows = {}};
}

}

TEST_F(BestTableTrackerTest, StartsWithNoModel) {
    EXPECT_FALSE(tracker.best_model().has_value());
    EXPECT_EQ(tracker.best_cost(), std::numeric_limits<size_t>::max());
}

TEST_F(BestTableTrackerTest, ACheaperModelIsRecorded) {
    tracker.try_record(first, 9);
    tracker.try_record(second, 4);
    EXPECT_EQ(tracker.best_model().value().graph, second.graph);
    EXPECT_EQ(tracker.best_cost(), 4u);
}

TEST_F(BestTableTrackerTest, AModelNoCheaperThanTheBestIsIgnored) {
    tracker.try_record(first, 4);
    tracker.try_record(second, 4);
    EXPECT_EQ(tracker.best_model().value().graph, first.graph);
}

TEST_F(BestTableTrackerTest, ACostlierModelLaterIsIgnored) {
    tracker.try_record(first, 3);
    tracker.try_record(second, 8);
    EXPECT_EQ(tracker.best_model().value().graph, first.graph);
    EXPECT_EQ(tracker.best_cost(), 3u);
}

TEST_F(BestTableTrackerTest, TheFirstRecordAlwaysWins) {
    tracker.try_record(first, 1000);
    EXPECT_EQ(tracker.best_cost(), 1000u);
}
