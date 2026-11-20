#include <gtest/gtest.h>
#include "infrastructure/application_log.hpp"

using namespace AML;

namespace {

struct ApplicationLogTest : public ::testing::Test {
    application_log log;
};

}

TEST_F(ApplicationLogTest, ANewLogIsEmpty) {
    EXPECT_EQ(log.count(), 0u);
}

TEST_F(ApplicationLogTest, EveryRecordIsCounted) {
    log.record(prim_application{.prim = 0, .args = {1, 2}});
    log.record(prim_application{.prim = 0, .args = {1, 2}});
    log.record(prim_application{.prim = 1, .args = {3}});
    EXPECT_EQ(log.count(), 3u);
}

TEST_F(ApplicationLogTest, OnlyTheGivenPrimitivesApplicationsStartingWithThePrefixAreCounted) {
    log.record(prim_application{.prim = 0, .args = {1, 2}});
    log.record(prim_application{.prim = 0, .args = {1, 3}});
    log.record(prim_application{.prim = 0, .args = {2, 1}});
    log.record(prim_application{.prim = 1, .args = {1, 2}});
    EXPECT_EQ(log.count_with_prefix(0, {}), 3u);
    EXPECT_EQ(log.count_with_prefix(0, {1}), 2u);
    EXPECT_EQ(log.count_with_prefix(0, {1, 3}), 1u);
    EXPECT_EQ(log.count_with_prefix(1, {2}), 0u);
}

TEST_F(ApplicationLogTest, ClearingForgetsEveryApplication) {
    log.record(prim_application{.prim = 0, .args = {1}});
    log.clear();
    EXPECT_EQ(log.count(), 0u);
    EXPECT_EQ(log.count_with_prefix(0, {}), 0u);
}

TEST_F(ApplicationLogTest, AFullTupleMatchesOnlyItself) {
    log.record(prim_application{.prim = 0, .args = {1, 2}});
    log.record(prim_application{.prim = 0, .args = {1, 3}});
    EXPECT_EQ(log.count_with_prefix(0, {1, 2}), 1u);
    EXPECT_EQ(log.count_with_prefix(0, {2, 2}), 0u);
}

TEST_F(ApplicationLogTest, AnUnusedPrimitiveCountsNothing) {
    log.record(prim_application{.prim = 0, .args = {1}});
    EXPECT_EQ(log.count_with_prefix(7, {}), 0u);
}

TEST_F(ApplicationLogTest, ANullaryApplicationIsCounted) {
    log.record(prim_application{.prim = 3, .args = {}});
    EXPECT_EQ(log.count_with_prefix(3, {}), 1u);
}
