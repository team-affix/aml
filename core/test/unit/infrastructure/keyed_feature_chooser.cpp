#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/keyed_feature_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMeasureKey {
    MOCK_METHOD(size_t, measure, (size_t), (const));
};

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct KeyedFeatureChooserTest : public ::testing::Test {
    NiceMock<MockMeasureKey> measure_key;
    NiceMock<MockChooseOption> choose_option;
    keyed_feature_chooser<NiceMock<MockMeasureKey>, NiceMock<MockChooseOption>> chooser{measure_key, choose_option};

    void SetUp() override {
        ON_CALL(measure_key, measure(0)).WillByDefault(Return(0));
        ON_CALL(measure_key, measure(1)).WillByDefault(Return(0));
        ON_CALL(measure_key, measure(2)).WillByDefault(Return(12));
    }
};

}

TEST_F(KeyedFeatureChooserTest, TheFeatureIsChosenByEveryFeaturesKeyInPositionOrder) {
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0, 0, 12})).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(3), 2u);
}

TEST_F(KeyedFeatureChooserTest, EachFeatureIsMeasuredAtItsPosition) {
    EXPECT_CALL(measure_key, measure(_)).Times(AnyNumber());
    EXPECT_CALL(measure_key, measure(0)).WillOnce(Return(0));
    EXPECT_CALL(measure_key, measure(1)).WillOnce(Return(0));
    chooser.choose(2);
}

TEST_F(KeyedFeatureChooserTest, ASingleFeatureIsStillChosenThrough) {
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0})).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(1), 0u);
}
