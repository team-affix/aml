#include "infrastructure/kite_runtime.hpp"
#include "debug_assert.hpp"

namespace AML {

kite_runtime::kite_runtime(
    schema& types,
    const std::vector<typed_primitive>& primitives,
    const std::vector<typed_constant>& constants,
    const bin& training,
    const kite_options& options)
    : manifest_(types, primitives, options)
    , training_(bin_view::of(training)) {
    manifest_.constant_seeder_.seed(constants);
}

episode_outcome kite_runtime::next() {
    return manifest_.mcts_episode_.run(training_);
}

const std::optional<model_node>& kite_runtime::best_model() const {
    return manifest_.best_model_tracker_.best_model();
}

size_t kite_runtime::best_cost() const {
    return manifest_.best_model_tracker_.best_cost();
}

std::optional<value> kite_runtime::infer(const std::vector<value>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.model_evaluator_.evaluate(best_model().value(), inputs);
}

}
