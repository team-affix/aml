#ifndef COLUMN_HASH_HPP
#define COLUMN_HASH_HPP

#include <cstddef>
#include "value_objects/column.hpp"

struct column_hash {
    size_t operator()(const column& cells) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

#endif
