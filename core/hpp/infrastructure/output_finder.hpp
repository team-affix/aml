#ifndef OUTPUT_FINDER_HPP
#define OUTPUT_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/feature.hpp"

// Finds the cheapest candidate whose column is the label column, so it can be a bin's output. The first wins a tie.
struct output_finder {
    std::optional<const feature*> find(const std::vector<candidate>& candidates, const column& labels) const;
};

#endif
