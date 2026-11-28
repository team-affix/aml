#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "value_objects/constant_id.hpp"
#include "infrastructure/child_pusher.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMeasureSetKey {
    MOCK_METHOD(uint64_t, measure, (const std::vector<feature_id>&), (const));
};

struct MockSizeAncestry {
    MOCK_METHOD(size_t, size, (const std::vector<feature_id>&, const std::vector<constant_id>&), (const));
};

struct MockTryRecordSetKey {
    MOCK_METHOD(bool, try_record, (uint64_t, size_t));
};

struct MockCountDistinctRows {
    MOCK_METHOD(size_t, count, (const std::vector<feature_id>&));
};

struct MockPushSet {
    MOCK_METHOD(void, push, (const queued_set&));
};

// The set {2, 5, 8} has key 77, an ancestry of size 9 and 3 distinct rows.
struct ChildPusherTest : public ::testing::Test {
    NiceMock<MockMeasureSetKey> measure_set_key;
    NiceMock<MockSizeAncestry> size_ancestry;
    NiceMock<MockTryRecordSetKey> try_record_set_key;
    NiceMock<MockCountDistinctRows> count_distinct_rows;
    NiceMock<MockPushSet> push_set;
    child_pusher<
        NiceMock<MockMeasureSetKey>, NiceMock<MockSizeAncestry>, NiceMock<MockTryRecordSetKey>,
        NiceMock<MockCountDistinctRows>, NiceMock<MockPushSet>>
        pusher{measure_set_key, size_ancestry, try_record_set_key, count_distinct_rows, push_set};

    void SetUp() override {
        ON_CALL(measure_set_key, measure(std::vector<feature_id>{2, 5, 8})).WillByDefault(Return(77));
        ON_CALL(size_ancestry, size(std::vector<feature_id>{2, 5, 8}, std::vector<constant_id>{}))
            .WillByDefault(Return(9));
        ON_CALL(count_distinct_rows, count(std::vector<feature_id>{2, 5, 8})).WillByDefault(Return(3));
    }
};

}

TEST_F(ChildPusherTest, ARecordedSetIsPushedWithItsAncestryAndRowsAsCostAndMembersInIncreasingId) {
    ON_CALL(try_record_set_key, try_record(77, 9)).WillByDefault(Return(true));
    EXPECT_CALL(push_set, push(queued_set{.members = {2, 5, 8}, .cost = 12}));
    pusher.push({8, 2, 5});
}

TEST_F(ChildPusherTest, ASetNotRecordedIsNeitherPushedNorCountedForRows) {
    ON_CALL(try_record_set_key, try_record(77, 9)).WillByDefault(Return(false));
    EXPECT_CALL(push_set, push(_)).Times(0);
    EXPECT_CALL(count_distinct_rows, count(_)).Times(0);
    pusher.push({5, 8, 2});
}

TEST_F(ChildPusherTest, TheKeyIsRecordedWithTheAncestrySize) {
    EXPECT_CALL(try_record_set_key, try_record(_, _)).Times(AnyNumber());
    EXPECT_CALL(try_record_set_key, try_record(77, 9)).WillOnce(Return(true));
    pusher.push({2, 5, 8});
}

TEST_F(ChildPusherTest, TheEmptySetCanBePushed) {
    ON_CALL(measure_set_key, measure(std::vector<feature_id>{})).WillByDefault(Return(0));
    ON_CALL(size_ancestry, size(std::vector<feature_id>{}, std::vector<constant_id>{})).WillByDefault(Return(0));
    ON_CALL(try_record_set_key, try_record(0, 0)).WillByDefault(Return(true));
    ON_CALL(count_distinct_rows, count(std::vector<feature_id>{})).WillByDefault(Return(1));
    EXPECT_CALL(push_set, push(queued_set{.members = {}, .cost = 1}));
    pusher.push({});
}
