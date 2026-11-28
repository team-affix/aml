#ifndef CONSTANT_COMPUTER_HPP
#define CONSTANT_COMPUTER_HPP

#include <any>
#include <variant>
#include <vector>
#include "value_objects/constant_derivation.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Computes a constant's one value from its derivation: an offered constant's own value, or a primitive applied to the
// operands' values (scalars, given in the order of the application's arguments) as one row, whose one cell it gives.
template<typename IGetNativePrimitive, typename IGetNativeType>
struct constant_computer {
    constant_computer(
        IGetNativePrimitive& get_native_primitive,
        IGetNativeType& get_native_type,
        const std::vector<native_constant>& offered);
    std::any compute(const constant_derivation& origin, const std::vector<operand_data>& operands) const;
private:
    IGetNativePrimitive& get_native_primitive_;
    IGetNativeType& get_native_type_;
    const std::vector<native_constant>& offered_;
};

#define CONSTANT_COMPUTER_TEMPLATE template<typename IGetNativePrimitive, typename IGetNativeType>
#define CONSTANT_COMPUTER          constant_computer<IGetNativePrimitive, IGetNativeType>

CONSTANT_COMPUTER_TEMPLATE
CONSTANT_COMPUTER::constant_computer(
    IGetNativePrimitive& get_native_primitive,
    IGetNativeType& get_native_type,
    const std::vector<native_constant>& offered)
    : get_native_primitive_(get_native_primitive)
    , get_native_type_(get_native_type)
    , offered_(offered) {}

CONSTANT_COMPUTER_TEMPLATE
std::any CONSTANT_COMPUTER::compute(
    const constant_derivation& origin, const std::vector<operand_data>& operands) const {
    if (const constant_derivation::atom* atom = std::get_if<constant_derivation::atom>(&origin.content))
        return offered_.at(atom->index).literal;
    const constant_derivation::application& application = std::get<constant_derivation::application>(origin.content);
    const native_primitive& primitive = get_native_primitive_.primitive(application.prim);
    return get_native_type_.type(primitive.result_type).cell(primitive.apply(operands, 1), 0);
}

#undef CONSTANT_COMPUTER
#undef CONSTANT_COMPUTER_TEMPLATE

}

#endif
