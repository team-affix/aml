#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/scrutinee_chooser.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockSynthesizeFeatures {
    MOCK_METHOD(const std::vector<candidate>&, synthesize, (const bin&));
};

struct MockChooseFeature {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct ScrutineeChooserTest : public ::testing::Test {
    ScrutineeChooserTest();
    value falsity;
    value truth;
    feature first;
    feature second;
    bin data;
    std::vector<candidate> features;
    NiceMock<MockSynthesizeFeatures> synthesize_features;
    NiceMock<MockChooseFeature> choose_feature;
    scrutinee_chooser<NiceMock<MockSynthesizeFeatures>, NiceMock<MockChooseFeature>> chooser{
        synthesize_features, choose_feature};

    void SetUp() override {
        ON_CALL(synthesize_features, synthesize(data)).WillByDefault(ReturnRef(features));
    }
};

ScrutineeChooserTest::ScrutineeChooserTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , first{feature::var{.index = 0}}
    , second{feature::var{.index = 1}}
    , data{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                     typed_column{.type = 0, .cells = {&truth, &falsity}}},
           .labels = {&falsity, &truth}}
    , features{
          candidate{.expression = &first, .cost = 1, .observed = data.scope[0]},
          candidate{.expression = &second, .cost = 1, .observed = data.scope[1]}}
    , synthesize_features()
    , choose_feature() {}

}

TEST_F(ScrutineeChooserTest, TheChosenFeatureOfTheBinsSessionIsTheScrutinee) {
    EXPECT_CALL(synthesize_features, synthesize(data)).WillOnce(ReturnRef(features));
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(data), features[1]);
}

TEST_F(ScrutineeChooserTest, TheScrutineeOutlivesTheSession) {
    ON_CALL(choose_feature, choose(2)).WillByDefault(Return(0));
    const candidate scrutinee = chooser.choose(data);
    features.clear();
    EXPECT_EQ(scrutinee.expression, &first);
    EXPECT_EQ(scrutinee.observed, data.scope[0]);
}
