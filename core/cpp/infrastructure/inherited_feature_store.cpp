#include "infrastructure/inherited_feature_store.hpp"

#include <utility>

namespace AML {

inherited_feature_store::inherited_feature_store()
    : by_rows_()
    , none_() {}

void inherited_feature_store::clear() {
    by_rows_.clear();
}

bool inherited_feature_store::has(const std::vector<size_t>* rows) const {
    return by_rows_.contains(rows);
}

void inherited_feature_store::record(const std::vector<size_t>* rows, std::vector<feature> features) {
    by_rows_[rows] = std::move(features);
}

const std::vector<feature>& inherited_feature_store::inherited(const std::vector<size_t>* rows) const {
    const auto found = by_rows_.find(rows);
    if (found == by_rows_.end())
        return none_;
    return found->second;
}

}
