#include "infrastructure/column_hash_index.hpp"

namespace AML {

column_hash_index::column_hash_index()
    : features_()
    , none_() {}

void column_hash_index::insert(size_t hash, feature_id feature) {
    features_[hash].push_back(feature);
}

const std::vector<feature_id>& column_hash_index::find(size_t hash) const {
    const auto found = features_.find(hash);
    if (found == features_.end())
        return none_;
    return found->second;
}

}
