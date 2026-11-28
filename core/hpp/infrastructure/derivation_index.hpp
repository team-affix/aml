#ifndef DERIVATION_INDEX_HPP
#define DERIVATION_INDEX_HPP

#include <optional>
#include <unordered_map>
#include "value_objects/derivation.hpp"
#include "value_objects/derivation_hash.hpp"
#include "value_objects/feature_id.hpp"

namespace AML {

// Stores which feature each derivation already made.
struct derivation_index {
    derivation_index();
    void insert(const derivation& origin, feature_id feature);
    std::optional<feature_id> find(const derivation& origin) const;
private:
    std::unordered_map<derivation, feature_id, derivation_hash> features_;
};

}

#endif
