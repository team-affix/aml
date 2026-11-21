#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstddef>
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// A feature together with its cost and the values it takes over a bin's rows. Comparable for equality only, since a
// column view is.
struct candidate {
    feature expression;
    size_t cost;
    column_view observed;
    bool operator==(const candidate&) const = default;
};

}

#endif
