#include "infrastructure/stratum_bucket_store.hpp"

#include <utility>
#include "debug_assert.hpp"

namespace AML {

stratum_bucket_store::stratum_bucket_store()
    : numbers_()
    , strata_()
    , buckets_() {}

void stratum_bucket_store::push(const set_stratum& stratum, const queued_set& entry) {
    const auto [found, inserted] = numbers_.try_emplace(stratum, buckets_.size());
    if (inserted) {
        strata_.push_back(stratum);
        buckets_.emplace_back();
    }
    buckets_[found->second].push_back(entry);
}

size_t stratum_bucket_store::bucket_count() const {
    return buckets_.size();
}

size_t stratum_bucket_store::bucket_size(size_t bucket) const {
    return buckets_.at(bucket).size();
}

queued_set stratum_bucket_store::take(size_t bucket, size_t position) {
    std::vector<queued_set>& sets = buckets_.at(bucket);
    DEBUG_ASSERT(position < sets.size());
    queued_set taken = std::move(sets[position]);
    if (position != sets.size() - 1)
        sets[position] = std::move(sets.back());
    sets.pop_back();
    if (!sets.empty())
        return taken;
    const size_t last = buckets_.size() - 1;
    numbers_.erase(strata_[bucket]);
    if (bucket != last) {
        buckets_[bucket] = std::move(buckets_[last]);
        strata_[bucket] = strata_[last];
        numbers_[strata_[bucket]] = bucket;
    }
    buckets_.pop_back();
    strata_.pop_back();
    return taken;
}

}
