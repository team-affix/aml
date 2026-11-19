#include "value_objects/feature_hash.hpp"

#include <functional>
#include <variant>

size_t feature_hash::operator()(const feature& expression) const noexcept {
    size_t seed = std::hash<size_t>{}(expression.content.index());
    if (const feature::var* variable = std::get_if<feature::var>(&expression.content))
        return hash_combine(seed, std::hash<var_id>{}(variable->index));
    if (const feature::constant* constant = std::get_if<feature::constant>(&expression.content))
        return hash_combine(seed, std::hash<const value*>{}(constant->literal));
    const feature::app& application = std::get<feature::app>(expression.content);
    seed = hash_combine(seed, std::hash<prim_id>{}(application.prim));
    for (const feature* arg : application.args)
        seed = hash_combine(seed, std::hash<const feature*>{}(arg));
    return seed;
}

size_t feature_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
