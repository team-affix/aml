#ifndef VALUE_HASH_HPP
#define VALUE_HASH_HPP

#include <cstddef>
#include "value_objects/value.hpp"

struct value_hash {
    size_t operator()(const value& term) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

#endif
