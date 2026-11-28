#include <gtest/gtest.h>
#include "infrastructure/visited_set_store.hpp"

using namespace AML;

namespace {

struct VisitedSetStoreTest : public ::testing::Test {
    visited_set_store visited;
};

}

TEST_F(VisitedSetStoreTest, AFirstVisitIsNew) {
    EXPECT_TRUE(visited.try_visit(7, 12));
}

TEST_F(VisitedSetStoreTest, ASecondVisitAtTheSameCostIsNot) {
    visited.try_visit(7, 12);
    EXPECT_FALSE(visited.try_visit(7, 12));
}

TEST_F(VisitedSetStoreTest, TheSameSetAtAnotherCostIsNew) {
    visited.try_visit(7, 12);
    EXPECT_TRUE(visited.try_visit(7, 10));
}

TEST_F(VisitedSetStoreTest, AnotherSetAtTheSameCostIsNew) {
    visited.try_visit(7, 12);
    EXPECT_TRUE(visited.try_visit(8, 12));
}
