#ifndef CHILD_PUSHER_HPP
#define CHILD_PUSHER_HPP

#include <algorithm>
#include <cstddef>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/queued_set.hpp"

namespace AML {

// Pushes a set onto the queue with its cost, its ancestry size plus its distinct rows, its members in increasing id.
// A set queued before is pushed again only once its ancestry is smaller than every time it was queued, so a cheaper
// origin found for a feature beneath it makes it new; its distinct rows never change.
template<
    typename IMeasureSetKey, typename ISizeAncestry, typename ITryRecordSetKey, typename ICountDistinctRows,
    typename IPushSet>
struct child_pusher {
    child_pusher(
        IMeasureSetKey& measure_set_key,
        ISizeAncestry& size_ancestry,
        ITryRecordSetKey& try_record_set_key,
        ICountDistinctRows& count_distinct_rows,
        IPushSet& push_set);
    void push(std::vector<feature_id> members);
private:
    IMeasureSetKey& measure_set_key_;
    ISizeAncestry& size_ancestry_;
    ITryRecordSetKey& try_record_set_key_;
    ICountDistinctRows& count_distinct_rows_;
    IPushSet& push_set_;
};

#define CHILD_PUSHER_TEMPLATE \
    template< \
        typename IMeasureSetKey, typename ISizeAncestry, typename ITryRecordSetKey, typename ICountDistinctRows, \
        typename IPushSet>
#define CHILD_PUSHER \
    child_pusher<IMeasureSetKey, ISizeAncestry, ITryRecordSetKey, ICountDistinctRows, IPushSet>

CHILD_PUSHER_TEMPLATE
CHILD_PUSHER::child_pusher(
    IMeasureSetKey& measure_set_key,
    ISizeAncestry& size_ancestry,
    ITryRecordSetKey& try_record_set_key,
    ICountDistinctRows& count_distinct_rows,
    IPushSet& push_set)
    : measure_set_key_(measure_set_key)
    , size_ancestry_(size_ancestry)
    , try_record_set_key_(try_record_set_key)
    , count_distinct_rows_(count_distinct_rows)
    , push_set_(push_set) {}

CHILD_PUSHER_TEMPLATE
void CHILD_PUSHER::push(std::vector<feature_id> members) {
    std::sort(members.begin(), members.end());
    const size_t ancestry_size = size_ancestry_.size(members, {});
    if (!try_record_set_key_.try_record(measure_set_key_.measure(members), ancestry_size))
        return;
    const size_t cost = ancestry_size + count_distinct_rows_.count(members);
    push_set_.push(queued_set{.members = std::move(members), .cost = cost});
}

#undef CHILD_PUSHER
#undef CHILD_PUSHER_TEMPLATE

}

#endif
