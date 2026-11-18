#include "infrastructure/cheapest_feature_index.hpp"

cheapest_feature_index::cheapest_feature_index()
    : candidates_()
    , slots_() {}

void cheapest_feature_index::clear() {
    slots_.clear();
    candidates_.clear();
}

void cheapest_feature_index::try_insert(const candidate& entry) {
    const auto [slot, inserted] = slots_.try_emplace(entry.observed.cells, candidates_.size());
    if (inserted) {
        candidates_.push_back(entry);
        return;
    }
    candidate& held = candidates_[slot->second];
    if (entry.cost >= held.cost)
        return;
    held = entry;
}

const std::vector<candidate>& cheapest_feature_index::candidates() const {
    return candidates_;
}
