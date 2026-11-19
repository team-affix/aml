#ifndef FEATURE_POOL_HPP
#define FEATURE_POOL_HPP

#include <unordered_set>
#include <vector>
#include "value_objects/feature.hpp"
#include "value_objects/feature_hash.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"
#include "value_objects/var_id.hpp"

// Interns features, so equal features share one pointer.
struct feature_pool {
    feature_pool();
    const feature* make_var(var_id index);
    const feature* make_app(prim_id prim, const std::vector<const feature*>& args);
    const feature* make_constant(const value* literal);
private:
    const feature* intern(feature&& expression);
    std::unordered_set<feature, feature_hash> features_;
};

#endif
