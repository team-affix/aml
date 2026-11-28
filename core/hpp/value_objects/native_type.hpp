#ifndef NATIVE_TYPE_HPP
#define NATIVE_TYPE_HPP

#include <any>
#include <cstddef>
#include <functional>
#include <typeindex>
#include <vector>
#include "infrastructure/native_cell_comparer.hpp"
#include "infrastructure/native_cell_reader.hpp"
#include "infrastructure/native_classifier.hpp"
#include "infrastructure/native_column_filler.hpp"
#include "infrastructure/native_column_hasher.hpp"

namespace AML {

// A C++ type a column solver's columns may hold, with the operations it needs on them, erased so columns of every type
// are handled alike. A column of the type is a std::vector of it in an any; a single value is the value in an any.
// cell_bytes is the bytes one cell holds in place. Not comparable, since it holds functions.
struct native_type {
    std::type_index type;
    size_t cell_bytes;
    std::function<std::vector<size_t>(const std::any&)> classify;
    std::function<size_t(const std::any&)> hash;
    std::function<std::any(const std::any&, size_t)> fill;
    std::function<bool(const std::any&, size_t, const std::any&, size_t)> equal_at;
    std::function<std::any(const std::any&, size_t)> cell;
    // The type T, whose values Hash hashes and == compares.
    template<typename T, typename Hash>
    static native_type of();
};

template<typename T, typename Hash>
native_type native_type::of() {
    return native_type{
        .type = std::type_index(typeid(T)),
        .cell_bytes = sizeof(T),
        .classify = native_classifier<T, Hash>(),
        .hash = native_column_hasher<T, Hash>(),
        .fill = native_column_filler<T>(),
        .equal_at = native_cell_comparer<T>(),
        .cell = native_cell_reader<T>()};
}

}

#endif
