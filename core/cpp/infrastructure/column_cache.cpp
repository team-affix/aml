#include "infrastructure/column_cache.hpp"
#include "debug_assert.hpp"

namespace AML {

column_cache::column_cache(size_t byte_budget)
    : byte_budget_(byte_budget)
    , held_bytes_(0)
    , recency_()
    , slots_() {}

void column_cache::insert(feature_id feature, std::shared_ptr<const cached_column> column, size_t bytes) {
    DEBUG_ASSERT(!slots_.contains(feature));
    recency_.push_front(feature);
    slots_.emplace(feature, slot{.column = std::move(column), .bytes = bytes, .recency = recency_.begin()});
    held_bytes_ += bytes;
    while (held_bytes_ > byte_budget_ && recency_.size() > 1) {
        const feature_id evicted = recency_.back();
        held_bytes_ -= slots_.at(evicted).bytes;
        slots_.erase(evicted);
        recency_.pop_back();
    }
}

std::shared_ptr<const cached_column> column_cache::find(feature_id feature) {
    const auto found = slots_.find(feature);
    if (found == slots_.end())
        return nullptr;
    recency_.splice(recency_.begin(), recency_, found->second.recency);
    return found->second.column;
}

size_t column_cache::held_bytes() const {
    return held_bytes_;
}

}
