#ifndef SCOUT_RUNTIME_HPP
#define SCOUT_RUNTIME_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "infrastructure/schema.hpp"
#include "infrastructure/scout_manifest.hpp"
#include "infrastructure/value_pool.hpp"
#include "value_objects/bin.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/scout_options.hpp"
#include "value_objects/typed_primitive.hpp"
#include "value_objects/value.hpp"

struct scout_runtime {
    scout_runtime(
        schema& types,
        value_pool& values,
        const std::vector<typed_primitive>& primitives,
        const bin& training,
        const scout_options& options);
    void next();
    const model_node* best_model() const;
    size_t best_cost() const;
    std::optional<const value*> infer(const std::vector<const value*>& inputs) const;
private:
    scout_manifest manifest_;
    bin training_;
};

#endif
