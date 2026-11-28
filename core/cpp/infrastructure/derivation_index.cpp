#include "infrastructure/derivation_index.hpp"

namespace AML {

derivation_index::derivation_index()
    : features_() {}

void derivation_index::insert(const derivation& origin, feature_id feature) {
    features_.insert_or_assign(origin, feature);
}

std::optional<feature_id> derivation_index::find(const derivation& origin) const {
    const auto found = features_.find(origin);
    if (found == features_.end())
        return std::nullopt;
    return found->second;
}

}
