#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_sizer.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature*), (const));
};

struct MockSizeValue {
    MOCK_METHOD(size_t, size, (const value*), (const));
};

struct ModelSizerTest : public ::testing::Test {
    ModelSizerTest();
    value label;
    feature scrutinee;
    model_node leaf;
    model_node empty;
    NiceMock<MockSizeFeature> size_feature;
    NiceMock<MockSizeValue> size_value;
    model_sizer<NiceMock<MockSizeFeature>, NiceMock<MockSizeValue>> sizer{size_feature, size_value};

    void SetUp() override {
        ON_CALL(size_feature, size(&scrutinee)).WillByDefault(Return(3));
        ON_CALL(size_value, size(&label)).WillByDefault(Return(2));
    }
};

ModelSizerTest::ModelSizerTest()
    : label{.ctor = 0, .fields = {}}
    , scrutinee{feature::var{.index = 0}}
    , leaf{model_node::leaf{.label = &label}}
    , empty{model_node::empty{}}
    , size_feature()
    , size_value() {}

}

TEST_F(ModelSizerTest, EmptyCostsNothing) {
    EXPECT_EQ(sizer.size(&empty), 0u);
}

TEST_F(ModelSizerTest, ALeafCostsItsLabelsSize) {
    EXPECT_EQ(sizer.size(&leaf), 2u);
}

TEST_F(ModelSizerTest, AMatchCostsOnePlusItsScrutineePlusItsBranches) {
    const model_node match{model_node::match{.scrutinee = &scrutinee, .branches = {&leaf, &empty, &leaf}}};
    EXPECT_EQ(sizer.size(&match), 1u + 3u + 2u + 0u + 2u);
}
