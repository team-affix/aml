#ifndef NATIVE_CELL_READER_HPP
#define NATIVE_CELL_READER_HPP

#include <any>
#include <cstddef>
#include <vector>

namespace AML {

// Reads one row of a column of T, a std::vector<T> in an any, into an any of its own.
template<typename T>
struct native_cell_reader {
    std::any operator()(const std::any& cells, size_t row) const;
};

#define NATIVE_CELL_READER_TEMPLATE template<typename T>
#define NATIVE_CELL_READER          native_cell_reader<T>

NATIVE_CELL_READER_TEMPLATE
std::any NATIVE_CELL_READER::operator()(const std::any& cells, size_t row) const {
    const T cell = std::any_cast<const std::vector<T>&>(cells)[row];
    return std::any(cell);
}

#undef NATIVE_CELL_READER
#undef NATIVE_CELL_READER_TEMPLATE

}

#endif
