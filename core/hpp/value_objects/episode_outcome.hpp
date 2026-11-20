#ifndef EPISODE_OUTCOME_HPP
#define EPISODE_OUTCOME_HPP

#include <cstddef>
#include "value_objects/model_node.hpp"

// What one episode produced: the model it built and that model's cost.
struct episode_outcome {
    model_node model;
    size_t cost;
    auto operator<=>(const episode_outcome&) const = default;
};

#endif
