#include <any>
#include <memory>
#include <typeindex>
#include <gtest/gtest.h>
#include "infrastructure/constant_cache.hpp"

using namespace AML;

namespace {

struct ConstantCacheTest : public ::testing::Test {
    ConstantCacheTest();
    std::shared_ptr<const cached_constant> constant_of(int cell);
    constant_cache cache;
};

ConstantCacheTest::ConstantCacheTest()
    : cache(10) {}

std::shared_ptr<const cached_constant> ConstantCacheTest::constant_of(int cell) {
    return std::make_shared<const cached_constant>(
        cached_constant{.type = std::type_index(typeid(int)), .cell = std::any(cell)});
}

}

TEST_F(ConstantCacheTest, AMissingValueIsNotFound) {
    EXPECT_EQ(cache.find(0), nullptr);
}

TEST_F(ConstantCacheTest, AnInsertedValueIsFound) {
    const std::shared_ptr<const cached_constant> cached = constant_of(1);
    cache.insert(3, cached, 4);
    EXPECT_EQ(cache.find(3), cached);
    EXPECT_EQ(cache.held_bytes(), 4u);
}

TEST_F(ConstantCacheTest, ValuesWithinTheBudgetAreAllKept) {
    cache.insert(0, constant_of(1), 5);
    cache.insert(1, constant_of(2), 5);
    EXPECT_NE(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_EQ(cache.held_bytes(), 10u);
}

TEST_F(ConstantCacheTest, GoingOverTheBudgetEvictsTheLeastRecentlyUsed) {
    cache.insert(0, constant_of(1), 4);
    cache.insert(1, constant_of(1), 4);
    cache.insert(2, constant_of(1), 4);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_NE(cache.find(2), nullptr);
    EXPECT_EQ(cache.held_bytes(), 8u);
}

TEST_F(ConstantCacheTest, FindingAValueMakesItTheMostRecentlyUsed) {
    cache.insert(0, constant_of(1), 4);
    cache.insert(1, constant_of(1), 4);
    cache.find(0);
    cache.insert(2, constant_of(1), 4);
    EXPECT_NE(cache.find(0), nullptr);
    EXPECT_EQ(cache.find(1), nullptr);
}

TEST_F(ConstantCacheTest, EvictionContinuesUntilTheBytesFit) {
    cache.insert(0, constant_of(1), 3);
    cache.insert(1, constant_of(1), 3);
    cache.insert(2, constant_of(1), 3);
    cache.insert(3, constant_of(1), 9);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_EQ(cache.find(1), nullptr);
    EXPECT_EQ(cache.find(2), nullptr);
    EXPECT_NE(cache.find(3), nullptr);
    EXPECT_EQ(cache.held_bytes(), 9u);
}

TEST_F(ConstantCacheTest, AValueLargerThanTheBudgetIsStillHeldAlone) {
    cache.insert(0, constant_of(1), 4);
    cache.insert(1, constant_of(1), 50);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_NE(cache.find(1), nullptr);
    EXPECT_EQ(cache.held_bytes(), 50u);
}

TEST_F(ConstantCacheTest, AnEvictedValueStaysAliveWhileHeld) {
    const std::shared_ptr<const cached_constant> held = constant_of(7);
    cache.insert(0, held, 6);
    cache.insert(1, constant_of(1), 6);
    EXPECT_EQ(cache.find(0), nullptr);
    EXPECT_EQ(std::any_cast<int>(held->cell), 7);
}
