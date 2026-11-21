#ifndef MERLIN_RUNTIME_HPP
#define MERLIN_RUNTIME_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "infrastructure/merlin_manifest.hpp"
#include "infrastructure/schema.hpp"
#include "value_objects/bin.hpp"
#include "value_objects/bin_view.hpp"
#include "value_objects/episode_outcome.hpp"
#include "value_objects/merlin_options.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/typed_constant.hpp"
#include "value_objects/typed_primitive.hpp"
#include "value_objects/value.hpp"

namespace AML {

struct merlin_runtime {
    merlin_runtime(
        schema& types,
        const std::vector<typed_primitive>& primitives,
        const std::vector<typed_constant>& constants,
        const bin& training,
        const merlin_options& options);
    episode_outcome next();
    const std::optional<model_node>& best_model() const;
    size_t best_cost() const;
    std::optional<value> infer(const std::vector<value>& inputs) const;
private:
    merlin_manifest manifest_;
    bin_view training_;
};

}

#endif
