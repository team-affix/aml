#include "infrastructure/cheapest_feature_index.hpp"

#include <utility>

namespace AML {

cheapest_feature_index::cheapest_feature_index()
    : candidates_()
    , slots_()
    , by_type_()
    , none_() {}

void cheapest_feature_index::clear() {
    by_type_.clear();
    slots_.clear();
    candidates_.clear();
}

void cheapest_feature_index::try_insert(candidate entry) {
    const size_t hashed = typed_column_hash{}(entry.observed);
    const auto [first, last] = slots_.equal_range(hashed);
    auto slot = first;
    while (slot != last && candidates_[slot->second].observed != entry.observed)
        ++slot;
    if (slot == last) {
        const type_id type = entry.observed.type;
        if (type >= by_type_.size())
            by_type_.resize(type + 1);
        by_type_[type].push_back(candidates_.size());
        slots_.emplace(hashed, candidates_.size());
        candidates_.push_back(std::move(entry));
        return;
    }
    candidate& held = candidates_[slot->second];
    if (entry.cost >= held.cost)
        return;
    held = std::move(entry);
}

const std::vector<candidate>& cheapest_feature_index::candidates() const {
    return candidates_;
}

const std::vector<size_t>& cheapest_feature_index::features_of_type(type_id type) const {
    if (type >= by_type_.size())
        return none_;
    return by_type_[type];
}

}
