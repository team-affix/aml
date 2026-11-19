#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_sizer.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature*), (const));
};

struct ModelSizerTest : public ::testing::Test {
    ModelSizerTest();
    value label;
    feature scrutinee;
    feature inner_scrutinee;
    feature constant_output;
    feature variable_output;
    model_node constant_leaf;
    model_node variable_leaf;
    model_node empty;
    NiceMock<MockSizeFeature> size_feature;
    model_sizer<NiceMock<MockSizeFeature>> sizer{size_feature};

    void SetUp() override {
        ON_CALL(size_feature, size(&scrutinee)).WillByDefault(Return(3));
        ON_CALL(size_feature, size(&inner_scrutinee)).WillByDefault(Return(5));
        ON_CALL(size_feature, size(&constant_output)).WillByDefault(Return(2));
        ON_CALL(size_feature, size(&variable_output)).WillByDefault(Return(1));
    }
};

ModelSizerTest::ModelSizerTest()
    : label{.ctor = 0, .args = {}}
    , scrutinee{feature::var{.index = 0}}
    , inner_scrutinee{feature::var{.index = 1}}
    , constant_output{feature::constant{.literal = &label}}
    , variable_output{feature::var{.index = 2}}
    , constant_leaf{model_node::leaf{.output = &constant_output}}
    , variable_leaf{model_node::leaf{.output = &variable_output}}
    , empty{model_node::empty{}}
    , size_feature() {}

}

TEST_F(ModelSizerTest, EmptyCostsNothing) {
    EXPECT_EQ(sizer.size(&empty), 0u);
}

TEST_F(ModelSizerTest, ALeafCostsItsOutputsSize) {
    EXPECT_EQ(sizer.size(&constant_leaf), 2u);
    EXPECT_EQ(sizer.size(&variable_leaf), 1u);
}

TEST_F(ModelSizerTest, AMatchCostsOnePlusItsScrutineePlusItsBranches) {
    const model_node match{
        model_node::match{.scrutinee = &scrutinee, .branches = {&constant_leaf, &empty, &variable_leaf}}};
    EXPECT_EQ(sizer.size(&match), 1u + 3u + 2u + 0u + 1u);
}

TEST_F(ModelSizerTest, NestedMatchesAddUp) {
    const model_node inner{model_node::match{.scrutinee = &inner_scrutinee, .branches = {&constant_leaf, &empty}}};
    const model_node outer{model_node::match{.scrutinee = &scrutinee, .branches = {&inner, &variable_leaf}}};
    EXPECT_EQ(sizer.size(&outer), (1u + 3u) + (1u + 5u + 2u + 0u) + 1u);
}

TEST_F(ModelSizerTest, ASharedSubtreeIsCountedOncePerUse) {
    const model_node match{model_node::match{.scrutinee = &scrutinee, .branches = {&constant_leaf, &constant_leaf}}};
    EXPECT_EQ(sizer.size(&match), 1u + 3u + 2u + 2u);
}
