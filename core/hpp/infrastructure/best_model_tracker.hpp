#ifndef BEST_MODEL_TRACKER_HPP
#define BEST_MODEL_TRACKER_HPP

#include <cstddef>
#include <optional>
#include "value_objects/model_node.hpp"

// Stores a copy of the cheapest model recorded so far. Before any record there is no model and the cost is the largest
// size_t. A cheaper model replaces the copy; the copy is taken only then, which is rare.
struct best_model_tracker {
    best_model_tracker();
    void try_record(const model_node& model, size_t cost);
    const std::optional<model_node>& best_model() const;
    size_t best_cost() const;
private:
    std::optional<model_node> best_model_;
    size_t best_cost_;
};

#endif
