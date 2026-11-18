#ifndef BEST_MODEL_TRACKER_HPP
#define BEST_MODEL_TRACKER_HPP

#include <cstddef>
#include "value_objects/model_node.hpp"

// Stores the cheapest model recorded so far. Before any record there is no model and the cost is the largest size_t.
struct best_model_tracker {
    best_model_tracker();
    void try_record(const model_node* model, size_t cost);
    const model_node* best_model() const;
    size_t best_cost() const;
private:
    const model_node* best_model_;
    size_t best_cost_;
};

#endif
