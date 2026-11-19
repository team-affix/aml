#ifndef PRIMITIVE_CALLER_HPP
#define PRIMITIVE_CALLER_HPP

#include <variant>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"

// Calls the primitive behind an id on argument values.
template<typename IGetPrimitive>
struct primitive_caller {
    primitive_caller(IGetPrimitive& get_primitive);
    const value* call(prim_id prim, const std::vector<const value*>& args) const;
private:
    IGetPrimitive& get_primitive_;
};

#define PRIMITIVE_CALLER_TEMPLATE template<typename IGetPrimitive>
#define PRIMITIVE_CALLER          primitive_caller<IGetPrimitive>

PRIMITIVE_CALLER_TEMPLATE
PRIMITIVE_CALLER::primitive_caller(IGetPrimitive& get_primitive)
    : get_primitive_(get_primitive) {}

PRIMITIVE_CALLER_TEMPLATE
const value* PRIMITIVE_CALLER::call(prim_id prim, const std::vector<const value*>& args) const {
    return std::visit(
        [&args](const auto& primitive) { return primitive->apply(args); }, get_primitive_.primitive_at(prim));
}

#undef PRIMITIVE_CALLER
#undef PRIMITIVE_CALLER_TEMPLATE

#endif
