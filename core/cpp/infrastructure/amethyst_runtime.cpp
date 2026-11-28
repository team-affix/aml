#include "infrastructure/amethyst_runtime.hpp"
#include "debug_assert.hpp"

namespace AML {

amethyst_runtime::amethyst_runtime(
    const std::vector<native_type>& types,
    const std::vector<native_primitive>& primitives,
    const std::vector<native_constant>& constants,
    const native_bin& training,
    const amethyst_options& options)
    : training_(training)
    , constants_(constants)
    , root_()
    , manifest_(training_, constants_, types, primitives, root_, options) {
    manifest_.constant_atom_seeder_.seed(constants_);
    root_ = manifest_.input_feature_seeder_.seed(training_.inputs);
}

table_outcome amethyst_runtime::next() {
    return manifest_.mcts_walk_episode_.run();
}

const std::optional<table_model>& amethyst_runtime::best_model() const {
    return manifest_.best_table_tracker_.best_model();
}

size_t amethyst_runtime::best_cost() const {
    return manifest_.best_table_tracker_.best_cost();
}

std::optional<std::any> amethyst_runtime::infer(const std::vector<std::any>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.table_model_evaluator_.evaluate(best_model().value(), inputs);
}

}
