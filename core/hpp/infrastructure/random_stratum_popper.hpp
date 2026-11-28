#ifndef RANDOM_STRATUM_POPPER_HPP
#define RANDOM_STRATUM_POPPER_HPP

#include <cstddef>
#include <optional>
#include <random>
#include "value_objects/queued_set.hpp"

namespace AML {

// Pops a set for exploration, as Type-GBFS's type-based open list does: a bucket uniformly at random, then a set
// uniformly at random within it. There is none when no bucket is left.
template<typename IGetBucketCount, typename IGetBucketSize, typename ITakeFromBucket, typename IRandomEngine>
struct random_stratum_popper {
    random_stratum_popper(
        IGetBucketCount& get_bucket_count,
        IGetBucketSize& get_bucket_size,
        ITakeFromBucket& take_from_bucket,
        IRandomEngine& random_engine);
    std::optional<queued_set> pop();
private:
    IGetBucketCount& get_bucket_count_;
    IGetBucketSize& get_bucket_size_;
    ITakeFromBucket& take_from_bucket_;
    IRandomEngine& random_engine_;
};

#define RANDOM_STRATUM_POPPER_TEMPLATE \
    template<typename IGetBucketCount, typename IGetBucketSize, typename ITakeFromBucket, typename IRandomEngine>
#define RANDOM_STRATUM_POPPER \
    random_stratum_popper<IGetBucketCount, IGetBucketSize, ITakeFromBucket, IRandomEngine>

RANDOM_STRATUM_POPPER_TEMPLATE
RANDOM_STRATUM_POPPER::random_stratum_popper(
    IGetBucketCount& get_bucket_count,
    IGetBucketSize& get_bucket_size,
    ITakeFromBucket& take_from_bucket,
    IRandomEngine& random_engine)
    : get_bucket_count_(get_bucket_count)
    , get_bucket_size_(get_bucket_size)
    , take_from_bucket_(take_from_bucket)
    , random_engine_(random_engine) {}

RANDOM_STRATUM_POPPER_TEMPLATE
std::optional<queued_set> RANDOM_STRATUM_POPPER::pop() {
    const size_t bucket_count = get_bucket_count_.bucket_count();
    if (bucket_count == 0)
        return std::nullopt;
    std::uniform_int_distribution<size_t> buckets(0, bucket_count - 1);
    const size_t bucket = buckets(random_engine_);
    std::uniform_int_distribution<size_t> positions(0, get_bucket_size_.bucket_size(bucket) - 1);
    return take_from_bucket_.take(bucket, positions(random_engine_));
}

#undef RANDOM_STRATUM_POPPER
#undef RANDOM_STRATUM_POPPER_TEMPLATE

}

#endif
