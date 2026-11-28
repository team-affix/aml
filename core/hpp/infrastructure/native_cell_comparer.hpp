#ifndef NATIVE_CELL_COMPARER_HPP
#define NATIVE_CELL_COMPARER_HPP

#include <any>
#include <cstddef>
#include <vector>

namespace AML {

// Tells whether a row of one column of T equals a row of another; each column is a std::vector<T> in an any.
template<typename T>
struct native_cell_comparer {
    bool operator()(const std::any& left, size_t left_row, const std::any& right, size_t right_row) const;
};

#define NATIVE_CELL_COMPARER_TEMPLATE template<typename T>
#define NATIVE_CELL_COMPARER          native_cell_comparer<T>

NATIVE_CELL_COMPARER_TEMPLATE
bool NATIVE_CELL_COMPARER::operator()(
    const std::any& left, size_t left_row, const std::any& right, size_t right_row) const {
    const T& left_cell = std::any_cast<const std::vector<T>&>(left)[left_row];
    const T& right_cell = std::any_cast<const std::vector<T>&>(right)[right_row];
    return left_cell == right_cell;
}

#undef NATIVE_CELL_COMPARER
#undef NATIVE_CELL_COMPARER_TEMPLATE

}

#endif
