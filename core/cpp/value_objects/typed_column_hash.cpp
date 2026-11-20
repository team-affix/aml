#include "value_objects/typed_column_hash.hpp"

#include <functional>

namespace AML {

size_t typed_column_hash::operator()(const typed_column& observed) const noexcept {
    size_t seed = std::hash<type_id>{}(observed.type);
    seed = hash_combine(seed, std::hash<size_t>{}(observed.cells.size()));
    for (const value* cell : observed.cells)
        seed = hash_combine(seed, std::hash<const value*>{}(cell));
    return seed;
}

size_t typed_column_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}
