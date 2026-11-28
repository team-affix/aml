#include "infrastructure/granite_runtime.hpp"
#include "debug_assert.hpp"

namespace AML {

granite_runtime::granite_runtime(
    const std::vector<native_type>& types,
    const std::vector<native_primitive>& primitives,
    const std::vector<native_constant>& constants,
    const native_bin& training,
    const granite_options& options)
    : training_(training)
    , constants_(constants)
    , manifest_(training_, constants_, types, primitives, options) {
    manifest_.constant_atom_seeder_.seed(constants_);
    manifest_.child_pusher_.push(manifest_.input_feature_seeder_.seed(training_.inputs));
}

std::optional<table_outcome> granite_runtime::next() {
    return manifest_.cheapest_set_visitor_.visit();
}

const std::optional<table_model>& granite_runtime::best_model() const {
    return manifest_.best_table_tracker_.best_model();
}

size_t granite_runtime::best_cost() const {
    return manifest_.best_table_tracker_.best_cost();
}

std::optional<std::any> granite_runtime::infer(const std::vector<std::any>& inputs) const {
    DEBUG_ASSERT(best_model().has_value());
    return manifest_.table_model_evaluator_.evaluate(best_model().value(), inputs);
}

}
