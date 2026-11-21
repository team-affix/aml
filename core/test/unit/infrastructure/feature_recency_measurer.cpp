#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_recency_measurer.hpp"

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

struct MockGetCreationTime {
    MOCK_METHOD(size_t, creation_time, (size_t), (const));
};

// Position 0 holds a variable, 1 an application, 2 another application.
struct FeatureRecencyMeasurerTest : public ::testing::Test {
    FeatureRecencyMeasurerTest();
    candidate make(const feature& expression);
    value truth;
    std::vector<candidate> features;
    NiceMock<MockGetFeatures> get_features;
    NiceMock<MockGetCreationTime> get_creation_time;
    feature_recency_measurer<NiceMock<MockGetFeatures>, NiceMock<MockGetCreationTime>> measurer{
        get_features, get_creation_time};

    void SetUp() override {
        ON_CALL(get_features, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(get_creation_time, creation_time(0)).WillByDefault(Return(1));
        ON_CALL(get_creation_time, creation_time(1)).WillByDefault(Return(7));
        ON_CALL(get_creation_time, creation_time(2)).WillByDefault(Return(4));
    }
};

FeatureRecencyMeasurerTest::FeatureRecencyMeasurerTest()
    : truth{.ctor = 1, .args = {}}
    , features{
          make(feature{feature::var{.index = 0}}),
          make(feature{feature::app{.prim = 0, .args = {feature{feature::var{.index = 0}}}}}),
          make(feature{feature::app{.prim = 1, .args = {feature{feature::var{.index = 0}}}}})}
    , get_features()
    , get_creation_time() {}

candidate FeatureRecencyMeasurerTest::make(const feature& expression) {
    return candidate{
        .expression = expression, .cost = 1, .observed = column_view::of(typed_column{.type = 0, .cells = {truth}})};
}

}

TEST_F(FeatureRecencyMeasurerTest, AVariableIsZeroWhateverItsCreationTime) {
    EXPECT_EQ(measurer.measure(0), 0u);
}

TEST_F(FeatureRecencyMeasurerTest, ASynthesizedFeatureIsItsCreationTime) {
    EXPECT_EQ(measurer.measure(1), 7u);
    EXPECT_EQ(measurer.measure(2), 4u);
}

TEST_F(FeatureRecencyMeasurerTest, TheCreationTimeIsReadAtTheFeaturesPosition) {
    EXPECT_CALL(get_creation_time, creation_time(_)).Times(AnyNumber());
    EXPECT_CALL(get_creation_time, creation_time(2)).WillOnce(Return(9));
    EXPECT_EQ(measurer.measure(2), 9u);
}

TEST_F(FeatureRecencyMeasurerTest, AVariablesCreationTimeIsNotRead) {
    EXPECT_CALL(get_creation_time, creation_time(_)).Times(0);
    measurer.measure(0);
}
