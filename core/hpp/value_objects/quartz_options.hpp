#ifndef QUARTZ_OPTIONS_HPP
#define QUARTZ_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// adds_per_feature caps the additions a visited set tries per feature in its ancestry. queue_capacity bounds the sets
// waiting in the queue; the costliest are dropped past it. column_cache_bytes is the budget for cached feature columns,
// constant_cache_bytes for cached constant values.
struct quartz_options {
    size_t adds_per_feature;
    size_t queue_capacity;
    size_t column_cache_bytes;
    size_t constant_cache_bytes;
    uint32_t seed;
    auto operator<=>(const quartz_options&) const = default;
};

}

#endif
