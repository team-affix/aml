#ifndef TYPED_COLUMN_HASH_HPP
#define TYPED_COLUMN_HASH_HPP

#include <cstddef>
#include "value_objects/typed_column.hpp"

namespace AML {

// Hashes a typed column by its type and its cells. Values carry no type, so the type tells apart columns whose cells
// look the same.
struct typed_column_hash {
    size_t operator()(const typed_column& observed) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

}

#endif
