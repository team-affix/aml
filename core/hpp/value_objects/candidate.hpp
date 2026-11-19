#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstddef>
#include "value_objects/feature.hpp"
#include "value_objects/typed_column.hpp"

// A feature together with its cost and the values it takes over a bin's rows.
struct candidate {
    feature expression;
    size_t cost;
    typed_column observed;
    auto operator<=>(const candidate&) const = default;
};

#endif
