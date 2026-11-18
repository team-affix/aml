#ifndef FEATURE_HASH_HPP
#define FEATURE_HASH_HPP

#include <cstddef>
#include "value_objects/feature.hpp"

struct feature_hash {
    size_t operator()(const feature& expression) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

#endif
