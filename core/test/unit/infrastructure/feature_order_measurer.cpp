#include <gtest/gtest.h>
#include "infrastructure/feature_order_measurer.hpp"

using namespace AML;

namespace {

struct FeatureOrderMeasurerTest : public ::testing::Test {
    FeatureOrderMeasurerTest();
    value four;
    feature x0;
    feature x1;
    feature four_constant;
    feature_order_measurer measurer;
};

FeatureOrderMeasurerTest::FeatureOrderMeasurerTest()
    : four{.ctor = 0, .args = {}}
    , x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , four_constant{feature::constant{.literal = four}}
    , measurer() {}

feature apply(prim_id prim, std::vector<feature> args) {
    return feature{feature::app{.prim = prim, .args = std::move(args)}};
}

}

TEST_F(FeatureOrderMeasurerTest, AVariableIsOrderZero) {
    EXPECT_EQ(measurer.measure(x0), 0u);
}

TEST_F(FeatureOrderMeasurerTest, AConstantIsOrderZero) {
    EXPECT_EQ(measurer.measure(four_constant), 0u);
}

TEST_F(FeatureOrderMeasurerTest, AnApplicationOfOrderZeroFeaturesIsOrderOne) {
    EXPECT_EQ(measurer.measure(apply(0, {x0, four_constant})), 1u);
}

TEST_F(FeatureOrderMeasurerTest, ANullaryApplicationIsOrderOne) {
    EXPECT_EQ(measurer.measure(apply(0, {})), 1u);
}

TEST_F(FeatureOrderMeasurerTest, AnApplicationIsOneMoreThanItsHighestOrderArgument) {
    const feature first_order = apply(0, {x0, x1});
    EXPECT_EQ(measurer.measure(apply(1, {first_order, x1})), 2u);
}

TEST_F(FeatureOrderMeasurerTest, TheHighestOrderArgumentCountsWhereverItIs) {
    const feature second_order = apply(0, {apply(0, {x0, x1}), x0});
    EXPECT_EQ(measurer.measure(apply(1, {x1, apply(2, {x0}), second_order})), 3u);
}

TEST_F(FeatureOrderMeasurerTest, OrderFollowsDepthNotSize) {
    const feature first_order = apply(0, {x0, x1});
    EXPECT_EQ(measurer.measure(apply(1, {first_order, first_order})), 2u);
}
