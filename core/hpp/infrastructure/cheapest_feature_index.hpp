#ifndef CHEAPEST_FEATURE_INDEX_HPP
#define CHEAPEST_FEATURE_INDEX_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/type_id.hpp"

namespace AML {

// Stores the cheapest candidate seen for each observed column (its type and the values it reads), in insertion order
// and by type. A cheaper candidate for a column already held replaces the held one in place, so a replacement never
// changes a position's type. Each position also holds its creation time: every new column takes the next time, from
// 1, and a replacement keeps the time of the column it replaces, so a larger time is a more recently created feature.
struct cheapest_feature_index {
    cheapest_feature_index();
    void clear();
    void try_insert(candidate entry);
    const std::vector<candidate>& candidates() const;
    const std::vector<size_t>& features_of_type(type_id type) const;
    size_t creation_time(size_t position) const;
private:
    std::vector<candidate> candidates_;
    // Positions by their column's hash; a hash's positions are compared with the held columns, so no column is copied
    // as a key.
    std::unordered_multimap<size_t, size_t> slots_;
    std::vector<std::vector<size_t>> by_type_;
    std::vector<size_t> none_;
    std::vector<size_t> creation_times_;
    size_t last_creation_time_;
};

}

#endif
