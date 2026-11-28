#include "infrastructure/set_queue.hpp"

namespace AML {

set_queue::set_queue(size_t capacity)
    : capacity_(capacity)
    , sets_() {}

void set_queue::push(const queued_set& entry) {
    sets_.emplace(entry.cost, entry.members);
    if (sets_.size() > capacity_)
        sets_.erase(std::prev(sets_.end()));
}

std::optional<queued_set> set_queue::pop() {
    if (sets_.empty())
        return std::nullopt;
    const auto cheapest = sets_.begin();
    const queued_set entry{.members = cheapest->second, .cost = cheapest->first};
    sets_.erase(cheapest);
    return entry;
}

size_t set_queue::size() const {
    return sets_.size();
}

}
