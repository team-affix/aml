#include "infrastructure/osprey_runtime.hpp"
#include "debug_assert.hpp"

osprey_runtime::osprey_runtime(
    schema& types,
    value_pool& values,
    const std::vector<typed_primitive>& primitives,
    const std::vector<typed_constant>& constants,
    const bin& training,
    const osprey_options& options)
    : manifest_(types, values, primitives, options)
    , training_(training) {
    manifest_.constant_seeder_.seed(constants);
}

episode_outcome osprey_runtime::next() {
    return manifest_.mcts_episode_.run(training_);
}

const std::optional<model_node>& osprey_runtime::best_model() const {
    return manifest_.best_model_tracker_.best_model();
}

size_t osprey_runtime::best_cost() const {
    return manifest_.best_model_tracker_.best_cost();
}

size_t osprey_runtime::commitment_count() const {
    return manifest_.commitments_.commitment_count();
}

std::optional<const value*> osprey_runtime::infer(const std::vector<const value*>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.model_evaluator_.evaluate(best_model().value(), inputs);
}
