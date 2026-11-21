#include <random>
#include <gtest/gtest.h>
#include "infrastructure/fraction_drawer.hpp"

using namespace AML;

namespace {

struct FractionDrawerTest : public ::testing::Test {
    FractionDrawerTest();
    std::mt19937 random_engine;
    fraction_drawer<std::mt19937> drawer;
};

FractionDrawerTest::FractionDrawerTest()
    : random_engine(3)
    , drawer(random_engine) {}

}

TEST_F(FractionDrawerTest, EveryFractionIsAtLeastZeroAndBelowOne) {
    for (int draw = 0; draw < 10000; ++draw) {
        const double fraction = drawer.draw();
        EXPECT_GE(fraction, 0.0);
        EXPECT_LT(fraction, 1.0);
    }
}

TEST_F(FractionDrawerTest, FractionsSpreadOverTheWholeInterval) {
    int below_a_quarter = 0;
    int above_three_quarters = 0;
    for (int draw = 0; draw < 10000; ++draw) {
        const double fraction = drawer.draw();
        below_a_quarter += fraction < 0.25 ? 1 : 0;
        above_three_quarters += fraction >= 0.75 ? 1 : 0;
    }
    EXPECT_NEAR(below_a_quarter, 2500, 200);
    EXPECT_NEAR(above_three_quarters, 2500, 200);
}

TEST_F(FractionDrawerTest, TheSameEngineStateDrawsTheSameFractions) {
    std::mt19937 other_engine(3);
    fraction_drawer<std::mt19937> other(other_engine);
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(drawer.draw(), other.draw());
}
