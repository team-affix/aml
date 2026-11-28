#ifndef GRANITE_OPTIONS_HPP
#define GRANITE_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// adds_per_feature caps the additions a visited set tries per feature in its ancestry. queue_capacity bounds the sets
// waiting in the cheapest-first queue; the costliest are dropped past it, though the exploring buckets keep them.
// column_cache_bytes is the budget for cached feature columns, constant_cache_bytes for cached constant values.
struct granite_options {
    size_t adds_per_feature;
    size_t queue_capacity;
    size_t column_cache_bytes;
    size_t constant_cache_bytes;
    uint32_t seed;
    auto operator<=>(const granite_options&) const = default;
};

}

#endif
