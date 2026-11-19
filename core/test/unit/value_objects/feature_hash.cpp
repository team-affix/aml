#include <gtest/gtest.h>
#include "value_objects/feature_hash.hpp"

namespace {

struct FeatureHashTest : public ::testing::Test {
    FeatureHashTest();
    value zero;
    feature first_var;
    feature second_var;
    feature_hash hasher;
};

FeatureHashTest::FeatureHashTest()
    : zero{.ctor = 0, .fields = {}}
    , first_var{feature::var{.index = 0}}
    , second_var{feature::var{.index = 1}}
    , hasher() {}

}

TEST_F(FeatureHashTest, EqualFeaturesHashEqual) {
    EXPECT_EQ(hasher(feature{feature::app{.prim = 3, .args = {&first_var, &second_var}}}),
              hasher(feature{feature::app{.prim = 3, .args = {&first_var, &second_var}}}));
}

TEST_F(FeatureHashTest, ArgumentOrderChangesTheHash) {
    EXPECT_NE(hasher(feature{feature::app{.prim = 3, .args = {&first_var, &second_var}}}),
              hasher(feature{feature::app{.prim = 3, .args = {&second_var, &first_var}}}));
}

TEST_F(FeatureHashTest, AVariableAndAConstantHashApart) {
    EXPECT_NE(hasher(feature{feature::var{.index = 0}}), hasher(feature{feature::constant{.literal = nullptr}}));
}

TEST_F(FeatureHashTest, DifferentConstantsHashApart) {
    const value one{.ctor = 1, .fields = {&zero}};
    EXPECT_NE(hasher(feature{feature::constant{.literal = &zero}}),
              hasher(feature{feature::constant{.literal = &one}}));
}

TEST_F(FeatureHashTest, DifferentVariablesHashApart) {
    EXPECT_NE(hasher(first_var), hasher(second_var));
}

TEST_F(FeatureHashTest, ThePrimitiveChangesTheHash) {
    EXPECT_NE(hasher(feature{feature::app{.prim = 0, .args = {&first_var}}}),
              hasher(feature{feature::app{.prim = 1, .args = {&first_var}}}));
}

TEST_F(FeatureHashTest, AnApplicationAndAVariableHashApart) {
    EXPECT_NE(hasher(feature{feature::app{.prim = 0, .args = {}}}), hasher(first_var));
}
