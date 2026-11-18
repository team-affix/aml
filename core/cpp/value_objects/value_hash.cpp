#include "value_objects/value_hash.hpp"

#include <functional>

size_t value_hash::operator()(const value& term) const noexcept {
    size_t seed = std::hash<ctor_id>{}(term.ctor);
    seed = hash_combine(seed, std::hash<size_t>{}(term.fields.size()));
    for (const value* field : term.fields)
        seed = hash_combine(seed, std::hash<const value*>{}(field));
    return seed;
}

size_t value_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
