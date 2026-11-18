#include "value_objects/column_hash.hpp"

#include <functional>

size_t column_hash::operator()(const column& cells) const noexcept {
    size_t seed = std::hash<size_t>{}(cells.size());
    for (const value* cell : cells)
        seed = hash_combine(seed, std::hash<const value*>{}(cell));
    return seed;
}

size_t column_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
