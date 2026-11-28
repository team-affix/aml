#ifndef GRANITE_RUNTIME_HPP
#define GRANITE_RUNTIME_HPP

#include <any>
#include <cstddef>
#include <optional>
#include <vector>
#include "infrastructure/granite_manifest.hpp"
#include "value_objects/native_bin.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/granite_options.hpp"
#include "value_objects/table_model.hpp"
#include "value_objects/table_outcome.hpp"

namespace AML {

// Learns over native C++ values: `types` gives the operations of every type a column holds, inputs, labels and
// primitive results alike. Each next() visits a set, alternately the cheapest queued and one picked to explore, and
// returns its model and cost; there is none once every queued set has been visited. infer() takes one value per input
// and gives the label, if the best model has one.
struct granite_runtime {
    granite_runtime(
        const std::vector<native_type>& types,
        const std::vector<native_primitive>& primitives,
        const std::vector<native_constant>& constants,
        const native_bin& training,
        const granite_options& options);
    std::optional<table_outcome> next();
    const std::optional<table_model>& best_model() const;
    size_t best_cost() const;
    std::optional<std::any> infer(const std::vector<std::any>& inputs) const;
private:
    native_bin training_;
    std::vector<native_constant> constants_;
    granite_manifest manifest_;
};

}

#endif
