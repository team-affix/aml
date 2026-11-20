#include "infrastructure/best_model_tracker.hpp"

#include <limits>

namespace AML {

best_model_tracker::best_model_tracker()
    : best_model_()
    , best_cost_(std::numeric_limits<size_t>::max()) {}

void best_model_tracker::try_record(const model_node& model, size_t cost) {
    if (cost >= best_cost_)
        return;
    best_model_ = model;
    best_cost_ = cost;
}

const std::optional<model_node>& best_model_tracker::best_model() const {
    return best_model_;
}

size_t best_model_tracker::best_cost() const {
    return best_cost_;
}

}
