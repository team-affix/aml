#ifndef OPERAND_APPLIER_HPP
#define OPERAND_APPLIER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/column.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"

namespace AML {

// Applies a primitive row by row to a tuple of operands.
template<typename IGetOperandCell, typename ICallPrimitive>
struct operand_applier {
    operand_applier(IGetOperandCell& get_operand_cell, ICallPrimitive& call_primitive);
    column apply(prim_id prim, const std::vector<size_t>& args, size_t row_count) const;
private:
    IGetOperandCell& get_operand_cell_;
    ICallPrimitive& call_primitive_;
};

#define OPERAND_APPLIER_TEMPLATE template<typename IGetOperandCell, typename ICallPrimitive>
#define OPERAND_APPLIER          operand_applier<IGetOperandCell, ICallPrimitive>

OPERAND_APPLIER_TEMPLATE
OPERAND_APPLIER::operand_applier(IGetOperandCell& get_operand_cell, ICallPrimitive& call_primitive)
    : get_operand_cell_(get_operand_cell)
    , call_primitive_(call_primitive) {}

OPERAND_APPLIER_TEMPLATE
column OPERAND_APPLIER::apply(prim_id prim, const std::vector<size_t>& args, size_t row_count) const {
    column cells;
    cells.reserve(row_count);
    std::vector<const value*> row_args(args.size());
    for (size_t row = 0; row < row_count; ++row) {
        for (size_t position = 0; position < args.size(); ++position)
            row_args[position] = get_operand_cell_.cell(args[position], row);
        cells.push_back(call_primitive_.call(prim, row_args));
    }
    return cells;
}

#undef OPERAND_APPLIER
#undef OPERAND_APPLIER_TEMPLATE

}

#endif
