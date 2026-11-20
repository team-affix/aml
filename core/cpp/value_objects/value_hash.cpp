#include "value_objects/value_hash.hpp"

#include <functional>

namespace AML {

size_t value_hash::operator()(const value& term) const noexcept {
    size_t seed = std::hash<ctor_id>{}(term.ctor);
    seed = hash_combine(seed, std::hash<size_t>{}(term.args.size()));
    for (const value& argument : term.args)
        seed = hash_combine(seed, (*this)(argument));
    return seed;
}

size_t value_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}
