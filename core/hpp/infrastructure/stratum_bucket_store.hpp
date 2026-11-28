#ifndef STRATUM_BUCKET_STORE_HPP
#define STRATUM_BUCKET_STORE_HPP

#include <cstddef>
#include <map>
#include <vector>
#include "value_objects/queued_set.hpp"
#include "value_objects/set_stratum.hpp"

namespace AML {

// Stores queued sets in one bucket per stratum, with no capacity. Buckets are numbered 0..bucket_count()-1 and sets
// within a bucket 0..bucket_size()-1; taking a set removes it, and a bucket left empty is removed, the last bucket
// taking its number. A set within a bucket is likewise replaced by the bucket's last.
struct stratum_bucket_store {
    stratum_bucket_store();
    void push(const set_stratum& stratum, const queued_set& entry);
    size_t bucket_count() const;
    size_t bucket_size(size_t bucket) const;
    queued_set take(size_t bucket, size_t position);
private:
    std::map<set_stratum, size_t> numbers_;
    std::vector<set_stratum> strata_;
    std::vector<std::vector<queued_set>> buckets_;
};

}

#endif
