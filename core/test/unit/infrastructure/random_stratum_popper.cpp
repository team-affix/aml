#include <map>
#include <optional>
#include <random>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/random_stratum_popper.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetBucketCount {
    MOCK_METHOD(size_t, bucket_count, (), (const));
};

struct MockGetBucketSize {
    MOCK_METHOD(size_t, bucket_size, (size_t), (const));
};

struct MockTakeFromBucket {
    MOCK_METHOD(queued_set, take, (size_t, size_t));
};

using test_random_stratum_popper_t = random_stratum_popper<
    NiceMock<MockGetBucketCount>, NiceMock<MockGetBucketSize>, NiceMock<MockTakeFromBucket>, std::mt19937_64>;

// Bucket 0 holds one set, bucket 1 holds three. A taken set's members are its bucket and position.
struct RandomStratumPopperTest : public ::testing::Test {
    RandomStratumPopperTest();
    static queued_set taken(size_t bucket, size_t position);
    std::mt19937_64 random_engine;
    NiceMock<MockGetBucketCount> get_bucket_count;
    NiceMock<MockGetBucketSize> get_bucket_size;
    NiceMock<MockTakeFromBucket> take_from_bucket;
    test_random_stratum_popper_t popper{get_bucket_count, get_bucket_size, take_from_bucket, random_engine};

    void SetUp() override {
        ON_CALL(get_bucket_count, bucket_count()).WillByDefault(Return(2));
        ON_CALL(get_bucket_size, bucket_size(0)).WillByDefault(Return(1));
        ON_CALL(get_bucket_size, bucket_size(1)).WillByDefault(Return(3));
        ON_CALL(take_from_bucket, take(_, _)).WillByDefault(&RandomStratumPopperTest::taken);
    }
};

RandomStratumPopperTest::RandomStratumPopperTest()
    : random_engine(3)
    , get_bucket_count()
    , get_bucket_size()
    , take_from_bucket() {}

queued_set RandomStratumPopperTest::taken(size_t bucket, size_t position) {
    return queued_set{.members = {bucket, position}, .cost = 0};
}

}

TEST_F(RandomStratumPopperTest, NoBucketsGiveNothingAndTakeNothing) {
    ON_CALL(get_bucket_count, bucket_count()).WillByDefault(Return(0));
    EXPECT_CALL(take_from_bucket, take(_, _)).Times(0);
    EXPECT_FALSE(popper.pop().has_value());
}

TEST_F(RandomStratumPopperTest, ALoneBucketsLoneSetIsTaken) {
    ON_CALL(get_bucket_count, bucket_count()).WillByDefault(Return(1));
    EXPECT_CALL(take_from_bucket, take(0, 0));
    EXPECT_EQ(popper.pop(), taken(0, 0));
}

TEST_F(RandomStratumPopperTest, BucketsAreDrawnUniformlyWhateverTheirSizes) {
    std::map<size_t, size_t> by_bucket;
    for (size_t draw = 0; draw < 4000; ++draw)
        ++by_bucket[popper.pop().value().members[0]];
    // a set-uniform draw would favor bucket 1 three to one
    EXPECT_GT(by_bucket[0], 1800u);
    EXPECT_GT(by_bucket[1], 1800u);
}

TEST_F(RandomStratumPopperTest, EverySetOfTheDrawnBucketCanBeTaken) {
    std::set<size_t> positions;
    for (size_t draw = 0; draw < 200; ++draw) {
        const queued_set popped = popper.pop().value();
        if (popped.members[0] == 1)
            positions.insert(popped.members[1]);
    }
    EXPECT_EQ(positions, (std::set<size_t>{0, 1, 2}));
}
