#ifndef ARGUMENT_OPTION_FINDER_HPP
#define ARGUMENT_OPTION_FINDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

namespace AML {

// Finds the operands of a primitive's next argument type that, after a prefix of chosen operands, still leave an
// application witness. Only operands of that type are looked at, in operand order.
template<typename ICountOperandsOfType, typename IGetOperandOfType, typename IDetectWitness>
struct argument_option_finder {
    argument_option_finder(
        ICountOperandsOfType& count_operands_of_type,
        IGetOperandOfType& get_operand_of_type,
        IDetectWitness& detect_witness);
    std::vector<size_t> find(
        prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const;
private:
    ICountOperandsOfType& count_operands_of_type_;
    IGetOperandOfType& get_operand_of_type_;
    IDetectWitness& detect_witness_;
};

#define ARGUMENT_OPTION_FINDER_TEMPLATE template<typename ICOT, typename IGOT, typename IDW>
#define ARGUMENT_OPTION_FINDER          argument_option_finder<ICOT, IGOT, IDW>

ARGUMENT_OPTION_FINDER_TEMPLATE
ARGUMENT_OPTION_FINDER::argument_option_finder(
    ICOT& count_operands_of_type, IGOT& get_operand_of_type, IDW& detect_witness)
    : count_operands_of_type_(count_operands_of_type)
    , get_operand_of_type_(get_operand_of_type)
    , detect_witness_(detect_witness) {}

ARGUMENT_OPTION_FINDER_TEMPLATE
std::vector<size_t> ARGUMENT_OPTION_FINDER::find(
    prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const {
    DEBUG_ASSERT(prefix.size() < arg_types.size());
    const type_id next_type = arg_types[prefix.size()];
    const size_t operand_count = count_operands_of_type_.count_of_type(next_type);

    std::vector<size_t> options;
    std::vector<size_t> extended = prefix;
    extended.push_back(0);
    for (size_t position = 0; position < operand_count; ++position) {
        extended.back() = get_operand_of_type_.operand_of_type(next_type, position);
        if (!detect_witness_.detect(prim, arg_types, extended))
            continue;
        options.push_back(extended.back());
    }
    return options;
}

#undef ARGUMENT_OPTION_FINDER
#undef ARGUMENT_OPTION_FINDER_TEMPLATE

}

#endif
