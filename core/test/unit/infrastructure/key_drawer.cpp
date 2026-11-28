#include <cstdint>
#include <random>
#include <unordered_set>
#include <gtest/gtest.h>
#include "infrastructure/key_drawer.hpp"

using namespace AML;

namespace {

struct KeyDrawerTest : public ::testing::Test {
    KeyDrawerTest();
    std::mt19937_64 random_engine;
    key_drawer<std::mt19937_64> drawer;
};

KeyDrawerTest::KeyDrawerTest()
    : random_engine(3)
    , drawer(random_engine) {}

}

TEST_F(KeyDrawerTest, KeysDoNotRepeatOverManyDraws) {
    std::unordered_set<uint64_t> keys;
    for (int draw = 0; draw < 10000; ++draw)
        keys.insert(drawer.draw());
    EXPECT_EQ(keys.size(), 10000u);
}

TEST_F(KeyDrawerTest, KeysReachTheHighBits) {
    int high = 0;
    for (int draw = 0; draw < 10000; ++draw)
        high += (drawer.draw() >> 63) == 1 ? 1 : 0;
    EXPECT_NEAR(high, 5000, 300);
}

TEST_F(KeyDrawerTest, TheSameSeedDrawsTheSameKeys) {
    std::mt19937_64 other_engine(3);
    key_drawer<std::mt19937_64> other(other_engine);
    EXPECT_EQ(drawer.draw(), other.draw());
}
