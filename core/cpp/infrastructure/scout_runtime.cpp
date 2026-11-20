#include "infrastructure/scout_runtime.hpp"
#include "debug_assert.hpp"

namespace AML {

scout_runtime::scout_runtime(
    schema& types,
    const std::vector<typed_primitive>& primitives,
    const std::vector<typed_constant>& constants,
    const bin& training,
    const scout_options& options)
    : manifest_(types, primitives, options)
    , training_(training) {
    manifest_.constant_seeder_.seed(constants);
}

episode_outcome scout_runtime::next() {
    return manifest_.model_episode_.run(training_);
}

const std::optional<model_node>& scout_runtime::best_model() const {
    return manifest_.best_model_tracker_.best_model();
}

size_t scout_runtime::best_cost() const {
    return manifest_.best_model_tracker_.best_cost();
}

std::optional<value> scout_runtime::infer(const std::vector<value>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.model_evaluator_.evaluate(best_model().value(), inputs);
}

}
