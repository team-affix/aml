#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/tiered_feature_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetFeatures {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockMeasureOrder {
    MOCK_METHOD(size_t, measure, (const feature&), (const));
};

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct TieredFeatureChooserTest : public ::testing::Test {
    TieredFeatureChooserTest();
    feature x0;
    feature x1;
    feature derived;
    std::vector<candidate> features;
    NiceMock<MockGetFeatures> get_features;
    NiceMock<MockMeasureOrder> measure_order;
    NiceMock<MockChooseOption> choose_option;
    tiered_feature_chooser<NiceMock<MockGetFeatures>, NiceMock<MockMeasureOrder>, NiceMock<MockChooseOption>> chooser{
        get_features, measure_order, choose_option};

    void SetUp() override {
        ON_CALL(get_features, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(measure_order, measure(x0)).WillByDefault(Return(0));
        ON_CALL(measure_order, measure(x1)).WillByDefault(Return(0));
        ON_CALL(measure_order, measure(derived)).WillByDefault(Return(2));
    }
};

TieredFeatureChooserTest::TieredFeatureChooserTest()
    : x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , derived{feature::app{.prim = 0, .args = {x0, x1}}}
    , features{
          candidate{.expression = x0, .cost = 1, .observed = {}},
          candidate{.expression = derived, .cost = 3, .observed = {}},
          candidate{.expression = x1, .cost = 1, .observed = {}}} {}

}

TEST_F(TieredFeatureChooserTest, ChoosesByTheOrdersOfTheSessionsFeaturesInStoreOrder) {
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0, 2, 0})).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(3), 1u);
}

TEST_F(TieredFeatureChooserTest, ReturnsThePositionTheOptionChooserGives) {
    ON_CALL(choose_option, choose(_)).WillByDefault(Return(2));
    EXPECT_EQ(chooser.choose(3), 2u);
}

TEST_F(TieredFeatureChooserTest, ASingleFeatureIsStillChosenThrough) {
    features = {candidate{.expression = x1, .cost = 1, .observed = {}}};
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0})).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(1), 0u);
}

TEST_F(TieredFeatureChooserTest, ACountOtherThanTheStoresIsRejected) {
    EXPECT_THROW(chooser.choose(2), std::logic_error);
}
