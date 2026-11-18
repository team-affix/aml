#ifndef CHEAPEST_FEATURE_INDEX_HPP
#define CHEAPEST_FEATURE_INDEX_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/column_hash.hpp"

// Stores the cheapest candidate seen for each column. Candidates keep their insertion order; a cheaper candidate
// for a column already held replaces the held one in place. Keying on the column alone is enough because a value's
// constructor belongs to exactly one type.
struct cheapest_feature_index {
    cheapest_feature_index();
    void clear();
    void try_insert(candidate entry);
    const std::vector<candidate>& candidates() const;
private:
    std::vector<candidate> candidates_;
    std::unordered_map<column, size_t, column_hash> slots_;
};

#endif
