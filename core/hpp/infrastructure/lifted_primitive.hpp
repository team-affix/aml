#ifndef LIFTED_PRIMITIVE_HPP
#define LIFTED_PRIMITIVE_HPP

#include <any>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "infrastructure/operand_cell_reader.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// A native function lifted to columns: given one operand per argument, each a column (a std::vector of the argument's
// type in an any) or a scalar read at every row, it calls the function on every row's cells and returns the results
// as a std::vector<Ret> in an any. Each operand is unwrapped once, not once per cell.
template<typename Ret, typename... Args>
struct lifted_primitive {
    lifted_primitive(std::function<Ret(Args...)> function);
    std::any operator()(const std::vector<operand_data>& operands, size_t row_count) const;
private:
    template<size_t... Positions>
    std::any apply_rows(
        const std::vector<operand_data>& operands, size_t row_count, std::index_sequence<Positions...>) const;
    std::function<Ret(Args...)> function_;
};

#define LIFTED_PRIMITIVE_TEMPLATE template<typename Ret, typename... Args>
#define LIFTED_PRIMITIVE          lifted_primitive<Ret, Args...>

LIFTED_PRIMITIVE_TEMPLATE
LIFTED_PRIMITIVE::lifted_primitive(std::function<Ret(Args...)> function)
    : function_(std::move(function)) {}

LIFTED_PRIMITIVE_TEMPLATE
std::any LIFTED_PRIMITIVE::operator()(const std::vector<operand_data>& operands, size_t row_count) const {
    return apply_rows(operands, row_count, std::index_sequence_for<Args...>{});
}

LIFTED_PRIMITIVE_TEMPLATE
template<size_t... Positions>
std::any LIFTED_PRIMITIVE::apply_rows(
    const std::vector<operand_data>& operands, size_t row_count, std::index_sequence<Positions...>) const {
    const std::tuple<operand_cell_reader<std::decay_t<Args>>...> readers{
        operand_cell_reader<std::decay_t<Args>>(operands[Positions])...};
    std::vector<std::decay_t<Ret>> cells;
    cells.reserve(row_count);
    for (size_t row = 0; row < row_count; ++row)
        cells.push_back(function_(std::get<Positions>(readers).read(row)...));
    return std::any(std::move(cells));
}

#undef LIFTED_PRIMITIVE
#undef LIFTED_PRIMITIVE_TEMPLATE

}

#endif
