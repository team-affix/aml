#include <set>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/stratum_bucket_store.hpp"

using namespace AML;

namespace {

queued_set set_of(feature_id member, size_t cost) {
    return queued_set{.members = {member}, .cost = cost};
}

struct StratumBucketStoreTest : public ::testing::Test {
    StratumBucketStoreTest();
    set_stratum low;
    set_stratum high;
    stratum_bucket_store buckets;
};

StratumBucketStoreTest::StratumBucketStoreTest()
    : low{.ancestry_size = 2, .distinct_rows = 4}
    , high{.ancestry_size = 5, .distinct_rows = 4}
    , buckets() {}

}

TEST_F(StratumBucketStoreTest, AnEmptyStoreHasNoBuckets) {
    EXPECT_EQ(buckets.bucket_count(), 0u);
}

TEST_F(StratumBucketStoreTest, SetsOfOneStratumShareABucket) {
    buckets.push(low, set_of(1, 6));
    buckets.push(low, set_of(2, 6));
    EXPECT_EQ(buckets.bucket_count(), 1u);
    EXPECT_EQ(buckets.bucket_size(0), 2u);
}

TEST_F(StratumBucketStoreTest, EachStratumGetsItsOwnBucketInOrderOfFirstPush) {
    buckets.push(high, set_of(1, 9));
    buckets.push(low, set_of(2, 6));
    buckets.push(high, set_of(3, 9));
    EXPECT_EQ(buckets.bucket_count(), 2u);
    EXPECT_EQ(buckets.bucket_size(0), 2u);
    EXPECT_EQ(buckets.bucket_size(1), 1u);
}

TEST_F(StratumBucketStoreTest, StrataDifferingInDistinctRowsAloneAreApart) {
    buckets.push(low, set_of(1, 6));
    buckets.push(set_stratum{.ancestry_size = 2, .distinct_rows = 5}, set_of(2, 7));
    EXPECT_EQ(buckets.bucket_count(), 2u);
}

TEST_F(StratumBucketStoreTest, TakingASetRemovesItAndTheBucketsLastTakesItsPlace) {
    buckets.push(low, set_of(1, 6));
    buckets.push(low, set_of(2, 6));
    buckets.push(low, set_of(3, 6));
    EXPECT_EQ(buckets.take(0, 0), set_of(1, 6));
    EXPECT_EQ(buckets.bucket_size(0), 2u);
    EXPECT_EQ(buckets.take(0, 0), set_of(3, 6));
    EXPECT_EQ(buckets.take(0, 0), set_of(2, 6));
}

TEST_F(StratumBucketStoreTest, TakingTheLastSetOfABucketTakesItsOwn) {
    buckets.push(low, set_of(1, 6));
    buckets.push(low, set_of(2, 6));
    EXPECT_EQ(buckets.take(0, 1), set_of(2, 6));
    EXPECT_EQ(buckets.take(0, 0), set_of(1, 6));
}

TEST_F(StratumBucketStoreTest, AnEmptiedBucketIsRemovedAndTheLastBucketTakesItsNumber) {
    buckets.push(low, set_of(1, 6));
    buckets.push(high, set_of(2, 9));
    buckets.take(0, 0);
    EXPECT_EQ(buckets.bucket_count(), 1u);
    EXPECT_EQ(buckets.take(0, 0), set_of(2, 9));
    EXPECT_EQ(buckets.bucket_count(), 0u);
}

TEST_F(StratumBucketStoreTest, AStratumWhoseBucketWasRemovedGetsANewBucket) {
    buckets.push(low, set_of(1, 6));
    buckets.push(high, set_of(2, 9));
    buckets.take(0, 0);
    buckets.push(low, set_of(3, 6));
    buckets.push(high, set_of(4, 9));
    EXPECT_EQ(buckets.bucket_count(), 2u);
    EXPECT_EQ(buckets.bucket_size(0), 2u);
    EXPECT_EQ(buckets.take(1, 0), set_of(3, 6));
}

TEST_F(StratumBucketStoreTest, TheLastBucketEmptiedIsRemovedWithoutMovingOthers) {
    buckets.push(low, set_of(1, 6));
    buckets.push(high, set_of(2, 9));
    buckets.take(1, 0);
    EXPECT_EQ(buckets.bucket_count(), 1u);
    EXPECT_EQ(buckets.take(0, 0), set_of(1, 6));
}
