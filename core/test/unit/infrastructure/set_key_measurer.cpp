#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_key_measurer.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetFeatureRecord {
    MOCK_METHOD(const feature_record&, record, (feature_id), (const));
};

struct SetKeyMeasurerTest : public ::testing::Test {
    SetKeyMeasurerTest();
    std::vector<feature_record> records;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    set_key_measurer<NiceMock<MockGetFeatureRecord>> measurer{get_feature_record};

    void SetUp() override {
        for (feature_id feature = 0; feature < records.size(); ++feature)
            ON_CALL(get_feature_record, record(feature)).WillByDefault(ReturnRef(records[feature]));
    }
};

SetKeyMeasurerTest::SetKeyMeasurerTest()
    : records{
          feature_record{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(int), .key = 0b0011},
          feature_record{.origin = derivation{derivation::input{.index = 1}}, .type = typeid(int), .key = 0b0101},
          feature_record{.origin = derivation{derivation::input{.index = 2}}, .type = typeid(int), .key = 0b1000}}
    , get_feature_record() {}

}

TEST_F(SetKeyMeasurerTest, TheEmptySetsKeyIsZero) {
    EXPECT_EQ(measurer.measure({}), 0u);
}

TEST_F(SetKeyMeasurerTest, AOneMemberSetHasItsMembersKey) {
    EXPECT_EQ(measurer.measure({1}), 0b0101u);
}

TEST_F(SetKeyMeasurerTest, MembersKeysAreCombinedByExclusiveOr) {
    EXPECT_EQ(measurer.measure({0, 1, 2}), 0b1110u);
}

TEST_F(SetKeyMeasurerTest, OrderDoesNotChangeTheKey) {
    EXPECT_EQ(measurer.measure({2, 0, 1}), measurer.measure({0, 1, 2}));
}
