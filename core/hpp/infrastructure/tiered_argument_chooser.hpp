#ifndef TIERED_ARGUMENT_CHOOSER_HPP
#define TIERED_ARGUMENT_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

// Chooses a primitive's arguments one position at a time, each from the operands that still leave a witness, by the
// operands' orders: the highest-order operands are offered first, so a feature just derived is likely to be built on.
template<typename IFindArgumentOptions, typename IGetOperandExpression, typename IMeasureOrder, typename IChooseOption>
struct tiered_argument_chooser {
    tiered_argument_chooser(
        IFindArgumentOptions& find_argument_options,
        IGetOperandExpression& get_operand_expression,
        IMeasureOrder& measure_order,
        IChooseOption& choose_option);
    std::vector<size_t> choose(prim_id prim, const std::vector<type_id>& arg_types);
private:
    IFindArgumentOptions& find_argument_options_;
    IGetOperandExpression& get_operand_expression_;
    IMeasureOrder& measure_order_;
    IChooseOption& choose_option_;
};

#define TIERED_ARGUMENT_CHOOSER_TEMPLATE template<typename IFAO, typename IGOE, typename IMO, typename ICO>
#define TIERED_ARGUMENT_CHOOSER          tiered_argument_chooser<IFAO, IGOE, IMO, ICO>

TIERED_ARGUMENT_CHOOSER_TEMPLATE
TIERED_ARGUMENT_CHOOSER::tiered_argument_chooser(
    IFAO& find_argument_options, IGOE& get_operand_expression, IMO& measure_order, ICO& choose_option)
    : find_argument_options_(find_argument_options)
    , get_operand_expression_(get_operand_expression)
    , measure_order_(measure_order)
    , choose_option_(choose_option) {}

TIERED_ARGUMENT_CHOOSER_TEMPLATE
std::vector<size_t> TIERED_ARGUMENT_CHOOSER::choose(prim_id prim, const std::vector<type_id>& arg_types) {
    std::vector<size_t> args;
    while (args.size() < arg_types.size()) {
        const std::vector<size_t> options = find_argument_options_.find(prim, arg_types, args);
        DEBUG_ASSERT(!options.empty());
        std::vector<size_t> orders;
        for (const size_t operand : options)
            orders.push_back(measure_order_.measure(get_operand_expression_.expression(operand)));
        args.push_back(options.at(choose_option_.choose(orders)));
    }
    return args;
}

#undef TIERED_ARGUMENT_CHOOSER
#undef TIERED_ARGUMENT_CHOOSER_TEMPLATE

#endif
