#ifndef OPERAND_CELL_READER_HPP
#define OPERAND_CELL_READER_HPP

#include <any>
#include <cstddef>
#include <vector>
#include "value_objects/operand_data.hpp"

namespace AML {

// Reads an operand of T row by row: a column's cell at the row, or a scalar's one value at every row.
template<typename T>
struct operand_cell_reader {
    operand_cell_reader(const operand_data& operand);
    decltype(auto) read(size_t row) const;
private:
    const std::vector<T>* column_;
    const T* scalar_;
};

#define OPERAND_CELL_READER_TEMPLATE template<typename T>
#define OPERAND_CELL_READER          operand_cell_reader<T>

OPERAND_CELL_READER_TEMPLATE
OPERAND_CELL_READER::operand_cell_reader(const operand_data& operand)
    : column_(operand.scalar ? nullptr : &std::any_cast<const std::vector<T>&>(*operand.data))
    , scalar_(operand.scalar ? &std::any_cast<const T&>(*operand.data) : nullptr) {}

OPERAND_CELL_READER_TEMPLATE
decltype(auto) OPERAND_CELL_READER::read(size_t row) const {
    return scalar_ != nullptr ? *scalar_ : (*column_)[row];
}

#undef OPERAND_CELL_READER
#undef OPERAND_CELL_READER_TEMPLATE

}

#endif
