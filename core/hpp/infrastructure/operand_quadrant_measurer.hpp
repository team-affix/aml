#ifndef OPERAND_QUADRANT_MEASURER_HPP
#define OPERAND_QUADRANT_MEASURER_HPP

#include <cstddef>
#include <variant>
#include "value_objects/feature.hpp"
#include "value_objects/quadrant_key.hpp"

namespace AML {

// Measures an operand's quadrant key (see quadrant_key.hpp): made when its expression is a primitive application,
// given when it is a variable or a constant; fresh until the session uses it as an argument, consumed after.
template<typename IGetOperandExpression, typename ICheckUsed>
struct operand_quadrant_measurer {
    operand_quadrant_measurer(IGetOperandExpression& get_operand_expression, ICheckUsed& check_used);
    size_t measure(size_t operand) const;
private:
    IGetOperandExpression& get_operand_expression_;
    ICheckUsed& check_used_;
};

#define OPERAND_QUADRANT_MEASURER_TEMPLATE template<typename IGetOperandExpression, typename ICheckUsed>
#define OPERAND_QUADRANT_MEASURER          operand_quadrant_measurer<IGetOperandExpression, ICheckUsed>

OPERAND_QUADRANT_MEASURER_TEMPLATE
OPERAND_QUADRANT_MEASURER::operand_quadrant_measurer(
    IGetOperandExpression& get_operand_expression, ICheckUsed& check_used)
    : get_operand_expression_(get_operand_expression)
    , check_used_(check_used) {}

OPERAND_QUADRANT_MEASURER_TEMPLATE
size_t OPERAND_QUADRANT_MEASURER::measure(size_t operand) const {
    const bool made = std::holds_alternative<feature::app>(get_operand_expression_.expression(operand).content);
    const bool fresh = !check_used_.is_used(operand);
    return (fresh ? k_fresh_quadrant_bit : 0) + (made ? k_made_quadrant_bit : 0);
}

#undef OPERAND_QUADRANT_MEASURER
#undef OPERAND_QUADRANT_MEASURER_TEMPLATE

}

#endif
