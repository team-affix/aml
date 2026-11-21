#include "value_objects/column_view_hash.hpp"

#include <functional>
#include "value_objects/value_hash.hpp"

namespace AML {

size_t column_view_hash::operator()(const column_view& observed) const noexcept {
    size_t seed = std::hash<type_id>{}(observed.type);
    seed = hash_combine(seed, std::hash<size_t>{}(observed.rows->size()));
    const value_hash hash_value;
    for (const size_t row : *observed.rows)
        seed = hash_combine(seed, hash_value((*observed.cells)[row]));
    return seed;
}

size_t column_view_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}
