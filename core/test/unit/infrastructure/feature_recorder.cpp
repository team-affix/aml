#include <any>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_recorder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

struct MockDrawKey {
    MOCK_METHOD(uint64_t, draw, ());
};

struct MockInsertFeatureRecord {
    MOCK_METHOD(feature_id, insert, (const feature_record&));
};

struct MockInsertDerivation {
    MOCK_METHOD(void, insert, (const derivation&, feature_id));
};

struct MockInsertColumnHash {
    MOCK_METHOD(void, insert, (size_t, feature_id));
};

struct MockMeasureColumnBytes {
    MOCK_METHOD(size_t, measure, (const cached_column&), (const));
};

struct MockInsertCachedColumn {
    MOCK_METHOD(void, insert, (feature_id, std::shared_ptr<const cached_column>, size_t));
};

using test_feature_recorder_t = feature_recorder<
    NiceMock<MockDrawKey>, NiceMock<MockInsertFeatureRecord>, NiceMock<MockInsertDerivation>,
    NiceMock<MockInsertColumnHash>, NiceMock<MockMeasureColumnBytes>, NiceMock<MockInsertCachedColumn>>;

struct FeatureRecorderTest : public ::testing::Test {
    FeatureRecorderTest();
    derivation origin;
    std::shared_ptr<const cached_column> made;
    NiceMock<MockDrawKey> draw_key;
    NiceMock<MockInsertFeatureRecord> insert_feature_record;
    NiceMock<MockInsertDerivation> insert_derivation;
    NiceMock<MockInsertColumnHash> insert_column_hash;
    NiceMock<MockMeasureColumnBytes> measure_column_bytes;
    NiceMock<MockInsertCachedColumn> insert_cached_column;
    test_feature_recorder_t recorder{
        draw_key, insert_feature_record, insert_derivation, insert_column_hash, measure_column_bytes,
        insert_cached_column};

    void SetUp() override {
        ON_CALL(draw_key, draw()).WillByDefault(Return(99));
        ON_CALL(insert_feature_record, insert(_)).WillByDefault(Return(6));
        ON_CALL(measure_column_bytes, measure(_)).WillByDefault(Return(40));
    }
};

FeatureRecorderTest::FeatureRecorderTest()
    : origin{derivation::application{.prim = 1, .args = feature_args({0, 2})}}
    , made(std::make_shared<const cached_column>(cached_column{
          .type = typeid(int), .cells = std::any(std::vector<int>{4}), .classes = {0}, .hash = 31}))
    , draw_key()
    , insert_feature_record()
    , insert_derivation()
    , insert_column_hash()
    , measure_column_bytes()
    , insert_cached_column() {}

}

TEST_F(FeatureRecorderTest, TheFeatureIsAddedWithItsOriginTheColumnsTypeAndAFreshKey) {
    EXPECT_CALL(insert_feature_record, insert(feature_record{.origin = origin, .type = typeid(int), .key = 99}))
        .WillOnce(Return(6));
    EXPECT_EQ(recorder.record(origin, made), 6u);
}

TEST_F(FeatureRecorderTest, TheDerivationIsIndexed) {
    EXPECT_CALL(insert_derivation, insert(origin, 6));
    recorder.record(origin, made);
}

TEST_F(FeatureRecorderTest, TheColumnsHashIsIndexed) {
    EXPECT_CALL(insert_column_hash, insert(31, 6));
    recorder.record(origin, made);
}

TEST_F(FeatureRecorderTest, TheColumnIsCachedWithItsBytes) {
    EXPECT_CALL(insert_cached_column, insert(6, made, 40));
    recorder.record(origin, made);
}

TEST_F(FeatureRecorderTest, EachRecordDrawsOneKey) {
    EXPECT_CALL(draw_key, draw()).Times(2).WillRepeatedly(Return(1));
    recorder.record(origin, made);
    recorder.record(origin, made);
}
