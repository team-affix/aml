#include <any>
#include <memory>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/column_cache.hpp"

using namespace AML;

namespace {

struct ColumnCacheTest : public ::testing::Test {
    ColumnCacheTest();
    std::shared_ptr<const cached_column> column_of(size_t classes);
    column_cache cache;
};

ColumnCacheTest::ColumnCacheTest()
    : cache(10) {}

std::shared_ptr<const cached_column> ColumnCacheTest::column_of(size_t classes) {
    return std::make_shared<const cached_column>(cached_column{
        .type = std::type_index(typeid(int)), .cells = std::any(std::vector<int>(classes, 0)),
        .classes = std::vector<size_t>(classes, 0), .hash = 0});
}

}

TEST_F(ColumnCacheTest, AMissingColumnIsNotFound) {
    EXPECT_EQ(cache.find(0), nullptr);
}

TEST_F(ColumnCacheTest, AnInsertedColumnIsFound) {
    const std::shared_ptr<const cached_column> column = column_of(1);
    cache.insert(3, column, 4);
    EXPECT_EQ(cache.find(3), column);
    EXPECT_EQ(cache.held_bytes(), 4u);
}

TEST_F(ColumnCacheTest, ColumnsWithinTheBudgetAreAllKept) {
    cache.insert(0, column_of(1), 5);
    cache.insert(1, column_of(2), 5);
    EXPECT_NE(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_EQ(cache.held_bytes(), 10u);
}

TEST_F(ColumnCacheTest, GoingOverTheBudgetEvictsTheLeastRecentlyUsed) {
    cache.insert(0, column_of(1), 4);
    cache.insert(1, column_of(1), 4);
    cache.insert(2, column_of(1), 4);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_NE(cache.find(2), nullptr);
    EXPECT_EQ(cache.held_bytes(), 8u);
}

TEST_F(ColumnCacheTest, FindingAColumnMakesItTheMostRecentlyUsed) {
    cache.insert(0, column_of(1), 4);
    cache.insert(1, column_of(1), 4);
    cache.find(0);
    cache.insert(2, column_of(1), 4);
    EXPECT_NE(cache.find(0), nullptr);
    EXPECT_EQ(cache.find(1), nullptr);
}

TEST_F(ColumnCacheTest, EvictionContinuesUntilTheBytesFit) {
    cache.insert(0, column_of(1), 3);
    cache.insert(1, column_of(1), 3);
    cache.insert(2, column_of(1), 3);
    cache.insert(3, column_of(1), 9);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_EQ(cache.find(1), nullptr);
    EXPECT_EQ(cache.find(2), nullptr);
    EXPECT_NE(cache.find(3), nullptr);
    EXPECT_EQ(cache.held_bytes(), 9u);
}

TEST_F(ColumnCacheTest, AColumnLargerThanTheBudgetIsStillHeldAlone) {
    cache.insert(0, column_of(1), 2);
    cache.insert(1, column_of(1), 50);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_EQ(cache.held_bytes(), 50u);
}

TEST_F(ColumnCacheTest, AnEvictedColumnStaysAliveWhileItIsHeld) {
    const std::shared_ptr<const cached_column> held = column_of(3);
    cache.insert(0, held, 6);
    cache.insert(1, column_of(1), 6);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_EQ(held->classes.size(), 3u);
}
