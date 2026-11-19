#ifndef OUTPUT_FINDER_HPP
#define OUTPUT_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/feature.hpp"

// Finds the cheapest candidate whose typed column is the labels, so it can be a bin's output. The first wins a tie.
struct output_finder {
    std::optional<const feature*> find(const std::vector<candidate>& candidates, const typed_column& labels) const;
};

#endif
