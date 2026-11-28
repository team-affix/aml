#ifndef STRATIFIED_CHILD_PUSHER_HPP
#define STRATIFIED_CHILD_PUSHER_HPP

#include <algorithm>
#include <cstddef>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/queued_set.hpp"
#include "value_objects/set_stratum.hpp"

namespace AML {

// Pushes a set, its members in increasing id, with its cost, its ancestry size plus its distinct rows, onto the queue
// and into the bucket of its stratum. A set queued before is pushed again only once its ancestry is smaller than every
// time it was queued, so a cheaper origin found for a feature beneath it makes it new; its distinct rows never change.
template<
    typename IMeasureSetKey, typename ISizeAncestry, typename ITryRecordSetKey, typename ICountDistinctRows,
    typename IPushSet, typename IPushStratifiedSet>
struct stratified_child_pusher {
    stratified_child_pusher(
        IMeasureSetKey& measure_set_key,
        ISizeAncestry& size_ancestry,
        ITryRecordSetKey& try_record_set_key,
        ICountDistinctRows& count_distinct_rows,
        IPushSet& push_set,
        IPushStratifiedSet& push_stratified_set);
    void push(std::vector<feature_id> members);
private:
    IMeasureSetKey& measure_set_key_;
    ISizeAncestry& size_ancestry_;
    ITryRecordSetKey& try_record_set_key_;
    ICountDistinctRows& count_distinct_rows_;
    IPushSet& push_set_;
    IPushStratifiedSet& push_stratified_set_;
};

#define STRATIFIED_CHILD_PUSHER_TEMPLATE \
    template< \
        typename IMeasureSetKey, typename ISizeAncestry, typename ITryRecordSetKey, typename ICountDistinctRows, \
        typename IPushSet, typename IPushStratifiedSet>
#define STRATIFIED_CHILD_PUSHER \
    stratified_child_pusher< \
        IMeasureSetKey, ISizeAncestry, ITryRecordSetKey, ICountDistinctRows, IPushSet, IPushStratifiedSet>

STRATIFIED_CHILD_PUSHER_TEMPLATE
STRATIFIED_CHILD_PUSHER::stratified_child_pusher(
    IMeasureSetKey& measure_set_key,
    ISizeAncestry& size_ancestry,
    ITryRecordSetKey& try_record_set_key,
    ICountDistinctRows& count_distinct_rows,
    IPushSet& push_set,
    IPushStratifiedSet& push_stratified_set)
    : measure_set_key_(measure_set_key)
    , size_ancestry_(size_ancestry)
    , try_record_set_key_(try_record_set_key)
    , count_distinct_rows_(count_distinct_rows)
    , push_set_(push_set)
    , push_stratified_set_(push_stratified_set) {}

STRATIFIED_CHILD_PUSHER_TEMPLATE
void STRATIFIED_CHILD_PUSHER::push(std::vector<feature_id> members) {
    std::sort(members.begin(), members.end());
    const size_t ancestry_size = size_ancestry_.size(members, {});
    if (!try_record_set_key_.try_record(measure_set_key_.measure(members), ancestry_size))
        return;
    const size_t distinct_rows = count_distinct_rows_.count(members);
    const queued_set entry{.members = std::move(members), .cost = ancestry_size + distinct_rows};
    push_set_.push(entry);
    push_stratified_set_.push(set_stratum{.ancestry_size = ancestry_size, .distinct_rows = distinct_rows}, entry);
}

#undef STRATIFIED_CHILD_PUSHER
#undef STRATIFIED_CHILD_PUSHER_TEMPLATE

}

#endif
