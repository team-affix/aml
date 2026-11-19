#include "infrastructure/feature_pool.hpp"

feature_pool::feature_pool()
    : features_() {}

const feature* feature_pool::make_var(var_id index) {
    return intern(feature{feature::var{.index = index}});
}

const feature* feature_pool::make_app(prim_id prim, const std::vector<const feature*>& args) {
    return intern(feature{feature::app{.prim = prim, .args = args}});
}

const feature* feature_pool::make_constant(const value* literal) {
    return intern(feature{feature::constant{.literal = literal}});
}

const feature* feature_pool::intern(feature&& expression) {
    return &*features_.insert(std::move(expression)).first;
}
