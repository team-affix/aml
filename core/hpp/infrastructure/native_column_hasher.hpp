#ifndef NATIVE_COLUMN_HASHER_HPP
#define NATIVE_COLUMN_HASHER_HPP

#include <any>
#include <cstddef>
#include <functional>
#include <vector>

namespace AML {

// Hashes a column of T, held in an any as a std::vector<T>, by its length and each cell in order.
template<typename T, typename Hash>
struct native_column_hasher {
    size_t operator()(const std::any& cells) const;
};

#define NATIVE_COLUMN_HASHER_TEMPLATE template<typename T, typename Hash>
#define NATIVE_COLUMN_HASHER          native_column_hasher<T, Hash>

NATIVE_COLUMN_HASHER_TEMPLATE
size_t NATIVE_COLUMN_HASHER::operator()(const std::any& cells) const {
    const std::vector<T>& typed = std::any_cast<const std::vector<T>&>(cells);
    const Hash hash_cell;
    size_t seed = std::hash<size_t>{}(typed.size());
    for (const T& cell : typed)
        seed ^= hash_cell(cell) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

#undef NATIVE_COLUMN_HASHER
#undef NATIVE_COLUMN_HASHER_TEMPLATE

}

#endif
