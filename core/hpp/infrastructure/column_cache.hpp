#ifndef COLUMN_CACHE_HPP
#define COLUMN_CACHE_HPP

#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>
#include "value_objects/cached_column.hpp"
#include "value_objects/feature_id.hpp"

namespace AML {

// Stores features' cached columns within a byte budget. Finding a column makes it the most recently used; inserting
// one evicts the least recently used others until the held bytes fit the budget again, so a single column larger than
// the budget is still held. Columns are shared, so a reader keeps an evicted column alive while it holds it.
struct column_cache {
    column_cache(size_t byte_budget);
    void insert(feature_id feature, std::shared_ptr<const cached_column> column, size_t bytes);
    std::shared_ptr<const cached_column> find(feature_id feature);
    size_t held_bytes() const;
private:
    struct slot {
        std::shared_ptr<const cached_column> column;
        size_t bytes;
        std::list<feature_id>::iterator recency;
    };
    size_t byte_budget_;
    size_t held_bytes_;
    std::list<feature_id> recency_;
    std::unordered_map<feature_id, slot> slots_;
};

}

#endif
