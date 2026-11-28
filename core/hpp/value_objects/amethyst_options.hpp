#ifndef AMETHYST_OPTIONS_HPP
#define AMETHYST_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// max_moves caps the drops and additions one episode makes. exploration_constant is UCT's c, on the scale of rewards
// normalized to [0, 1]. column_cache_bytes is the budget for cached feature columns, constant_cache_bytes for cached
// constant values.
struct amethyst_options {
    size_t max_moves;
    double exploration_constant;
    size_t column_cache_bytes;
    size_t constant_cache_bytes;
    uint32_t seed;
    auto operator<=>(const amethyst_options&) const = default;
};

}

#endif
