#ifndef COLUMN_HASH_INDEX_HPP
#define COLUMN_HASH_INDEX_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "value_objects/feature_id.hpp"

namespace AML {

// Stores the features whose columns hash to each hash, in insertion order.
struct column_hash_index {
    column_hash_index();
    void insert(size_t hash, feature_id feature);
    const std::vector<feature_id>& find(size_t hash) const;
private:
    std::unordered_map<size_t, std::vector<feature_id>> features_;
    std::vector<feature_id> none_;
};

}

#endif
