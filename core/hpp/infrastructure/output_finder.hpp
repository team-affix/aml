#ifndef OUTPUT_FINDER_HPP
#define OUTPUT_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// Finds the cheapest candidate whose observed column is the labels, so it can be a bin's output. The first wins a tie.
struct output_finder {
    std::optional<feature> find(const std::vector<candidate>& candidates, const column_view& labels) const;
};

}

#endif
