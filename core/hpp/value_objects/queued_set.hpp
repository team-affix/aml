#ifndef QUEUED_SET_HPP
#define QUEUED_SET_HPP

#include <cstddef>
#include <vector>
#include "value_objects/feature_id.hpp"

namespace AML {

// A set of features waiting in the search queue, with its cost.
struct queued_set {
    std::vector<feature_id> members;
    size_t cost;
    auto operator<=>(const queued_set&) const = default;
};

}

#endif
