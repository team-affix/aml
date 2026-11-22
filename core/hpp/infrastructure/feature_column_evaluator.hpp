#ifndef FEATURE_COLUMN_EVALUATOR_HPP
#define FEATURE_COLUMN_EVALUATOR_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/column.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

namespace AML {

// Evaluates a primitive application over every row of a bin, as a column of the primitive's result type: a variable
// reads the bin's column, a constant is its literal, and an application calls its primitive row by row on its
// arguments' values.
template<typename IGetPrimitiveType, typename ICallPrimitive>
struct feature_column_evaluator {
    feature_column_evaluator(IGetPrimitiveType& get_primitive_type, ICallPrimitive& call_primitive);
    column_view evaluate(const feature& application, const bin_view& data) const;
private:
    column evaluate_cells(const feature& expression, const bin_view& data) const;

    IGetPrimitiveType& get_primitive_type_;
    ICallPrimitive& call_primitive_;
};

#define FEATURE_COLUMN_EVALUATOR_TEMPLATE template<typename IGetPrimitiveType, typename ICallPrimitive>
#define FEATURE_COLUMN_EVALUATOR          feature_column_evaluator<IGetPrimitiveType, ICallPrimitive>

FEATURE_COLUMN_EVALUATOR_TEMPLATE
FEATURE_COLUMN_EVALUATOR::feature_column_evaluator(
    IGetPrimitiveType& get_primitive_type, ICallPrimitive& call_primitive)
    : get_primitive_type_(get_primitive_type)
    , call_primitive_(call_primitive) {}

FEATURE_COLUMN_EVALUATOR_TEMPLATE
column_view FEATURE_COLUMN_EVALUATOR::evaluate(const feature& application, const bin_view& data) const {
    DEBUG_ASSERT(std::holds_alternative<feature::app>(application.content));
    const type_id type = get_primitive_type_.type(std::get<feature::app>(application.content).prim).result_type;
    return column_view::of(typed_column{.type = type, .cells = evaluate_cells(application, data)});
}

FEATURE_COLUMN_EVALUATOR_TEMPLATE
column FEATURE_COLUMN_EVALUATOR::evaluate_cells(const feature& expression, const bin_view& data) const {
    const size_t row_count = data.labels.rows->size();
    if (const feature::var* variable = std::get_if<feature::var>(&expression.content)) {
        const column_view& observed = data.inputs.at(variable->index);
        column cells;
        cells.reserve(row_count);
        for (size_t row = 0; row < row_count; ++row)
            cells.push_back((*observed.cells)[(*observed.rows)[row]]);
        return cells;
    }
    if (const feature::constant* constant = std::get_if<feature::constant>(&expression.content))
        return column(row_count, constant->literal);
    const feature::app& application = std::get<feature::app>(expression.content);
    std::vector<column> arguments;
    arguments.reserve(application.args.size());
    for (const feature& argument : application.args)
        arguments.push_back(evaluate_cells(argument, data));
    column cells;
    cells.reserve(row_count);
    std::vector<const value*> row_args(arguments.size());
    for (size_t row = 0; row < row_count; ++row) {
        for (size_t position = 0; position < arguments.size(); ++position)
            row_args[position] = &arguments[position][row];
        cells.push_back(call_primitive_.call(application.prim, row_args));
    }
    return cells;
}

#undef FEATURE_COLUMN_EVALUATOR
#undef FEATURE_COLUMN_EVALUATOR_TEMPLATE

}

#endif
