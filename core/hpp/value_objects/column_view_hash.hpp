#ifndef COLUMN_VIEW_HASH_HPP
#define COLUMN_VIEW_HASH_HPP

#include <cstddef>
#include "value_objects/column_view.hpp"

namespace AML {

// Hashes a column view by its type and the values it reads, so equal views hash equal whatever cells and rows they
// share. Values carry no type, so the type tells apart columns whose cells look the same.
struct column_view_hash {
    size_t operator()(const column_view& observed) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

}

#endif
