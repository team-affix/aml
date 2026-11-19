#include "value_objects/value_hash.hpp"

#include <functional>

size_t value_hash::operator()(const value& term) const noexcept {
    size_t seed = std::hash<type_id>{}(term.ctor.type);
    seed = hash_combine(seed, std::hash<size_t>{}(term.ctor.tag));
    seed = hash_combine(seed, std::hash<size_t>{}(term.args.size()));
    for (const value* argument : term.args)
        seed = hash_combine(seed, std::hash<const value*>{}(argument));
    return seed;
}

size_t value_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
