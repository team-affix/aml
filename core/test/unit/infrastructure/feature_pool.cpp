#include <gtest/gtest.h>
#include "infrastructure/feature_pool.hpp"

namespace {

struct FeaturePoolTest : public ::testing::Test {
    FeaturePoolTest();
    value zero;
    feature_pool features;
};

FeaturePoolTest::FeaturePoolTest()
    : zero{.ctor = 0, .fields = {}}
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
