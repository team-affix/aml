#include "infrastructure/best_table_tracker.hpp"

#include <limits>

namespace AML {

best_table_tracker::best_table_tracker()
    : best_model_()
    , best_cost_(std::numeric_limits<size_t>::max()) {}

void best_table_tracker::try_record(const table_model& model, size_t cost) {
    if (cost >= best_cost_)
        return;
    best_model_ = model;
    best_cost_ = cost;
}

const std::optional<table_model>& best_table_tracker::best_model() const {
    return best_model_;
}

size_t best_table_tracker::best_cost() const {
    return best_cost_;
}

}
