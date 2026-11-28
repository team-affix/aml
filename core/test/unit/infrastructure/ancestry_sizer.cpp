#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ancestry_sizer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindAncestry {
    MOCK_METHOD(std::vector<feature_id>, find, (const std::vector<feature_id>&), (const));
};

struct MockGetFeatureRecord {
    MOCK_METHOD(const feature_record&, record, (feature_id), (const));
};

struct MockFindConstantAncestry {
    MOCK_METHOD(std::vector<constant_id>, find, (const std::vector<constant_id>&), (const));
};

// Feature 0 is an input; feature 1 applies a primitive to feature 0 and constant 5.
struct AncestrySizerTest : public ::testing::Test {
    AncestrySizerTest();
    std::vector<feature_record> records;
    NiceMock<MockFindAncestry> find_ancestry;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    NiceMock<MockFindConstantAncestry> find_constant_ancestry;
    ancestry_sizer<NiceMock<MockFindAncestry>, NiceMock<MockGetFeatureRecord>, NiceMock<MockFindConstantAncestry>>
        sizer{find_ancestry, get_feature_record, find_constant_ancestry};

    void SetUp() override {
        for (feature_id feature = 0; feature < records.size(); ++feature)
            ON_CALL(get_feature_record, record(feature)).WillByDefault(ReturnRef(records[feature]));
        ON_CALL(find_ancestry, find(std::vector<feature_id>{1})).WillByDefault(Return(std::vector<feature_id>{0, 1}));
        ON_CALL(find_ancestry, find(std::vector<feature_id>{0})).WillByDefault(Return(std::vector<feature_id>{0}));
    }
};

AncestrySizerTest::AncestrySizerTest()
    : records{
          feature_record{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(int), .key = 0},
          feature_record{
              .origin = derivation{derivation::application{
                  .prim = 0,
                  .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 5}}}}},
              .type = typeid(int),
              .key = 0}}
    , find_ancestry()
    , get_feature_record()
    , find_constant_ancestry() {}

}

TEST_F(AncestrySizerTest, NothingHasNoSize) {
    EXPECT_EQ(sizer.size({}, {}), 0u);
}

TEST_F(AncestrySizerTest, FeaturesWithNoConstantsCountTheirAncestry) {
    EXPECT_EQ(sizer.size({0}, {}), 1u);
}

TEST_F(AncestrySizerTest, AConstantOperandAddsItsConstantAncestry) {
    EXPECT_CALL(find_constant_ancestry, find(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(find_constant_ancestry, find(std::vector<constant_id>{5}))
        .WillOnce(Return(std::vector<constant_id>{2, 3, 5}));
    EXPECT_EQ(sizer.size({1}, {}), 5u);
}

TEST_F(AncestrySizerTest, GivenConstantsJoinTheConstantRoots) {
    EXPECT_CALL(find_constant_ancestry, find(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(find_constant_ancestry, find(std::vector<constant_id>{7, 5}))
        .WillOnce(Return(std::vector<constant_id>{5, 7}));
    EXPECT_EQ(sizer.size({1}, {7}), 4u);
}
