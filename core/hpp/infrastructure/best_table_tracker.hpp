#ifndef BEST_TABLE_TRACKER_HPP
#define BEST_TABLE_TRACKER_HPP

#include <cstddef>
#include <optional>
#include "value_objects/table_model.hpp"

namespace AML {

// Stores a copy of the cheapest table model recorded so far. Before any record there is no model and the cost is the
// largest size_t. A cheaper model replaces the copy.
struct best_table_tracker {
    best_table_tracker();
    void try_record(const table_model& model, size_t cost);
    const std::optional<table_model>& best_model() const;
    size_t best_cost() const;
private:
    std::optional<table_model> best_model_;
    size_t best_cost_;
};

}

#endif
