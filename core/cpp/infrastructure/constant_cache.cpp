#include "infrastructure/constant_cache.hpp"
#include "debug_assert.hpp"

namespace AML {

constant_cache::constant_cache(size_t byte_budget)
    : byte_budget_(byte_budget)
    , held_bytes_(0)
    , recency_()
    , slots_() {}

void constant_cache::insert(constant_id constant, std::shared_ptr<const cached_constant> cached, size_t bytes) {
    DEBUG_ASSERT(!slots_.contains(constant));
    recency_.push_front(constant);
    slots_.emplace(constant, slot{.cached = std::move(cached), .bytes = bytes, .recency = recency_.begin()});
    held_bytes_ += bytes;
    while (held_bytes_ > byte_budget_ && recency_.size() > 1) {
        const constant_id evicted = recency_.back();
        held_bytes_ -= slots_.at(evicted).bytes;
        slots_.erase(evicted);
        recency_.pop_back();
    }
}

std::shared_ptr<const cached_constant> constant_cache::find(constant_id constant) {
    const auto found = slots_.find(constant);
    if (found == slots_.end())
        return nullptr;
    recency_.splice(recency_.begin(), recency_, found->second.recency);
    return found->second.cached;
}

size_t constant_cache::held_bytes() const {
    return held_bytes_;
}

}
