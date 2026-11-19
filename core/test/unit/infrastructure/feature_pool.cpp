#include <gtest/gtest.h>
#include "infrastructure/feature_pool.hpp"

namespace {

struct FeaturePoolTest : public ::testing::Test {
    FeaturePoolTest();
    value zero;
    feature_pool features;
};

FeaturePoolTest::FeaturePoolTest()
    : zero{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , features() {}

}

TEST_F(FeaturePoolTest, EqualVariablesShareOnePointer) {
    EXPECT_EQ(features.make_var(2), features.make_var(2));
    EXPECT_NE(features.make_var(2), features.make_var(3));
}

TEST_F(FeaturePoolTest, EqualApplicationsShareOnePointer) {
    const feature* argument = features.make_var(0);
    EXPECT_EQ(features.make_app(1, {argument}), features.make_app(1, {features.make_var(0)}));
    EXPECT_NE(features.make_app(1, {argument}), features.make_app(2, {argument}));
}

TEST_F(FeaturePoolTest, EqualConstantsShareOnePointer) {
    EXPECT_EQ(features.make_constant(&zero), features.make_constant(&zero));
}

TEST_F(FeaturePoolTest, AMadeApplicationHoldsItsPrimitiveAndArguments) {
    const feature* argument = features.make_var(0);
    EXPECT_EQ(*features.make_app(4, {argument}), (feature{feature::app{.prim = 4, .args = {argument}}}));
}

TEST_F(FeaturePoolTest, AVariableAndAConstantAreDistinct) {
    EXPECT_NE(static_cast<const void*>(features.make_var(0)), static_cast<const void*>(features.make_constant(&zero)));
}

TEST_F(FeaturePoolTest, ArgumentOrderMatters) {
    const feature* first = features.make_var(0);
    const feature* second = features.make_var(1);
    EXPECT_NE(features.make_app(0, {first, second}), features.make_app(0, {second, first}));
}

TEST_F(FeaturePoolTest, NestedApplicationsAreInternedThroughTheirArguments) {
    const feature* inner = features.make_app(0, {features.make_var(0)});
    EXPECT_EQ(features.make_app(1, {inner}), features.make_app(1, {features.make_app(0, {features.make_var(0)})}));
}

TEST_F(FeaturePoolTest, MadeVariablesAndConstantsHoldTheirContent) {
    EXPECT_EQ(*features.make_var(6), (feature{feature::var{.index = 6}}));
    EXPECT_EQ(*features.make_constant(&zero), (feature{feature::constant{.literal = &zero}}));
}
