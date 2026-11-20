#include "infrastructure/razor_runtime.hpp"
#include "debug_assert.hpp"

razor_runtime::razor_runtime(
    schema& types,
    value_pool& values,
    const std::vector<typed_primitive>& primitives,
    const std::vector<typed_constant>& constants,
    const bin& training,
    const razor_options& options)
    : manifest_(types, values, primitives, options)
    , training_(training) {
    manifest_.constant_seeder_.seed(constants);
}

episode_outcome razor_runtime::next() {
    return manifest_.mcts_episode_.run(training_);
}

const std::optional<model_node>& razor_runtime::best_model() const {
    return manifest_.best_model_tracker_.best_model();
}

size_t razor_runtime::best_cost() const {
    return manifest_.best_model_tracker_.best_cost();
}

std::optional<const value*> razor_runtime::infer(const std::vector<const value*>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.model_evaluator_.evaluate(best_model().value(), inputs);
}
