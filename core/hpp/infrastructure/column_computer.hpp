#ifndef COLUMN_COMPUTER_HPP
#define COLUMN_COMPUTER_HPP

#include <any>
#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/native_column.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Computes a feature's values over the training rows from its derivation: an input's own column, or a primitive
// applied to the operands' data, given in the order of the application's arguments.
template<typename IGetNativePrimitive>
struct column_computer {
    column_computer(
        IGetNativePrimitive& get_native_primitive, const std::vector<native_column>& inputs, size_t row_count);
    std::any compute(const derivation& origin, const std::vector<operand_data>& operands) const;
private:
    IGetNativePrimitive& get_native_primitive_;
    const std::vector<native_column>& inputs_;
    size_t row_count_;
};

#define COLUMN_COMPUTER_TEMPLATE template<typename IGetNativePrimitive>
#define COLUMN_COMPUTER          column_computer<IGetNativePrimitive>

COLUMN_COMPUTER_TEMPLATE
COLUMN_COMPUTER::column_computer(
    IGetNativePrimitive& get_native_primitive, const std::vector<native_column>& inputs, size_t row_count)
    : get_native_primitive_(get_native_primitive)
    , inputs_(inputs)
    , row_count_(row_count) {}

COLUMN_COMPUTER_TEMPLATE
std::any COLUMN_COMPUTER::compute(const derivation& origin, const std::vector<operand_data>& operands) const {
    if (const derivation::input* input = std::get_if<derivation::input>(&origin.content))
        return inputs_.at(input->index).cells;
    const derivation::application& application = std::get<derivation::application>(origin.content);
    return get_native_primitive_.primitive(application.prim).apply(operands, row_count_);
}

#undef COLUMN_COMPUTER
#undef COLUMN_COMPUTER_TEMPLATE

}

#endif
