#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ancestry_finder.hpp"
#include "infrastructure/ancestry_sizer.hpp"
#include "infrastructure/child_pusher.hpp"
#include "infrastructure/constant_ancestry_finder.hpp"
#include "infrastructure/constant_graph.hpp"
#include "infrastructure/feature_graph.hpp"
#include "infrastructure/set_key_measurer.hpp"
#include "infrastructure/set_key_store.hpp"
#include "infrastructure/set_queue.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockCountDistinctRows {
    MOCK_METHOD(size_t, count, (const std::vector<feature_id>&));
};

using ancestry_finder_t = ancestry_finder<feature_graph>;
using constant_ancestry_finder_t = constant_ancestry_finder<constant_graph>;
using ancestry_sizer_t = ancestry_sizer<ancestry_finder_t, feature_graph, constant_ancestry_finder_t>;
using set_key_measurer_t = set_key_measurer<feature_graph>;
using child_pusher_t = child_pusher<
    set_key_measurer_t, ancestry_sizer_t, set_key_store, NiceMock<MockCountDistinctRows>, set_queue>;

// Feature 0 is an input, 1 = p0(0) and 2 = p1(1), so feature 2's ancestry has size 3. Every set has 4 distinct rows.
struct SetRequeuingIntegrationTest : public ::testing::Test {
    SetRequeuingIntegrationTest();
    feature_record record_of(derivation origin, uint64_t key);
    derivation application(prim_id prim, feature_id argument);
    feature_graph features;
    constant_graph constants;
    set_key_store set_keys;
    set_queue queue;
    NiceMock<MockCountDistinctRows> count_distinct_rows;
    ancestry_finder_t find_ancestry{features};
    constant_ancestry_finder_t find_constant_ancestry{constants};
    ancestry_sizer_t size_ancestry{find_ancestry, features, find_constant_ancestry};
    set_key_measurer_t measure_set_key{features};
    child_pusher_t pusher{measure_set_key, size_ancestry, set_keys, count_distinct_rows, queue};

    void SetUp() override {
        features.insert(record_of(derivation{derivation::input{.index = 0}}, 11));
        features.insert(record_of(application(0, 0), 22));
        features.insert(record_of(application(1, 1), 33));
        ON_CALL(count_distinct_rows, count(_)).WillByDefault(Return(4));
    }
};

SetRequeuingIntegrationTest::SetRequeuingIntegrationTest()
    : features()
    , constants()
    , set_keys()
    , queue(100)
    , count_distinct_rows() {}

feature_record SetRequeuingIntegrationTest::record_of(derivation origin, uint64_t key) {
    return feature_record{.origin = std::move(origin), .type = typeid(bool), .key = key};
}

derivation SetRequeuingIntegrationTest::application(prim_id prim, feature_id argument) {
    return derivation{derivation::application{.prim = prim, .args = {operand{operand::from_feature{.id = argument}}}}};
}

}

TEST_F(SetRequeuingIntegrationTest, ASetIsQueuedOnceWhileItsAncestryIsUnchanged) {
    pusher.push({2});
    pusher.push({2});
    EXPECT_EQ(queue.pop(), (queued_set{.members = {2}, .cost = 7}));
    EXPECT_FALSE(queue.pop().has_value());
}

TEST_F(SetRequeuingIntegrationTest, AFeatureGivenACheaperOriginLetsItsSetBeQueuedAgainAtTheLowerCost) {
    pusher.push({2});
    queue.pop();
    features.replace_origin(2, application(2, 0));
    pusher.push({2});
    EXPECT_EQ(queue.pop(), (queued_set{.members = {2}, .cost = 6}));
}

TEST_F(SetRequeuingIntegrationTest, ASetRequeuedAtItsLowerCostIsNotQueuedAgain) {
    pusher.push({2});
    queue.pop();
    features.replace_origin(2, application(2, 0));
    pusher.push({2});
    queue.pop();
    pusher.push({2});
    EXPECT_FALSE(queue.pop().has_value());
}

TEST_F(SetRequeuingIntegrationTest, ACheaperOriginBeneathAMemberLetsTheSetBeQueuedAgain) {
    features.insert(record_of(application(3, 2), 44));
    pusher.push({3});
    queue.pop();
    features.replace_origin(2, application(2, 0));
    pusher.push({3});
    EXPECT_EQ(queue.pop(), (queued_set{.members = {3}, .cost = 7}));
}

TEST_F(SetRequeuingIntegrationTest, ACheaperOriginOutsideTheAncestryDoesNotRequeueTheSet) {
    features.insert(record_of(application(3, 0), 44));
    pusher.push({3});
    queue.pop();
    features.replace_origin(2, application(2, 0));
    pusher.push({3});
    EXPECT_FALSE(queue.pop().has_value());
}
