#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_sizer.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeValue {
    MOCK_METHOD(size_t, size, (const value*), (const));
};

struct FeatureSizerTest : public ::testing::Test {
    FeatureSizerTest();
    value four;
    feature variable;
    feature constant;
    NiceMock<MockSizeValue> size_value;
    feature_sizer<NiceMock<MockSizeValue>> sizer{size_value};

    void SetUp() override {
        ON_CALL(size_value, size(&four)).WillByDefault(Return(5));
    }
};

FeatureSizerTest::FeatureSizerTest()
    : four{.ctor = 0, .fields = {}}
    , variable{feature::var{.index = 0}}
    , constant{feature::constant{.literal = &four}}
    , size_value() {}

}

TEST_F(FeatureSizerTest, AVariableCostsOne) {
    EXPECT_EQ(sizer.size(&variable), 1u);
}

TEST_F(FeatureSizerTest, AConstantCostsItsValuesSize) {
    EXPECT_EQ(sizer.size(&constant), 5u);
}

TEST_F(FeatureSizerTest, AnApplicationCostsOnePlusItsArguments) {
    const feature inner{feature::app{.prim = 0, .args = {&variable}}};
    const feature outer{feature::app{.prim = 1, .args = {&inner, &constant}}};
    EXPECT_EQ(sizer.size(&outer), 1u + 2u + 5u);
}
