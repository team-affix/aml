#ifndef CHEAPEST_FEATURE_INDEX_HPP
#define CHEAPEST_FEATURE_INDEX_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/typed_column_hash.hpp"
#include "value_objects/type_id.hpp"

// Stores the cheapest candidate seen for each typed column, in insertion order and by type. A cheaper candidate for a
// typed column already held replaces the held one in place, so a replacement never changes a position's type.
struct cheapest_feature_index {
    cheapest_feature_index();
    void clear();
    void try_insert(candidate entry);
    const std::vector<candidate>& candidates() const;
    const std::vector<size_t>& features_of_type(type_id type) const;
private:
    std::vector<candidate> candidates_;
    std::unordered_map<typed_column, size_t, typed_column_hash> slots_;
    std::vector<std::vector<size_t>> by_type_;
    std::vector<size_t> none_;
};

#endif
