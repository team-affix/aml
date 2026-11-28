#ifndef AMETHYST_RUNTIME_HPP
#define AMETHYST_RUNTIME_HPP

#include <any>
#include <cstddef>
#include <optional>
#include <vector>
#include "infrastructure/amethyst_manifest.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_bin.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/amethyst_options.hpp"
#include "value_objects/table_model.hpp"
#include "value_objects/table_outcome.hpp"

namespace AML {

// Learns over native C++ values, as Quartz does, searching with UCT that remembers its best path. Each next() runs one
// episode and returns the cheapest set it passed through, as its model and cost. infer() takes one value per input and
// gives the label, if the best model has one.
struct amethyst_runtime {
    amethyst_runtime(
        const std::vector<native_type>& types,
        const std::vector<native_primitive>& primitives,
        const std::vector<native_constant>& constants,
        const native_bin& training,
        const amethyst_options& options);
    table_outcome next();
    const std::optional<table_model>& best_model() const;
    size_t best_cost() const;
    std::optional<std::any> infer(const std::vector<std::any>& inputs) const;
private:
    native_bin training_;
    std::vector<native_constant> constants_;
    std::vector<feature_id> root_;
    amethyst_manifest manifest_;
};

}

#endif
