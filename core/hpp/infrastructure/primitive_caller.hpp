#ifndef PRIMITIVE_CALLER_HPP
#define PRIMITIVE_CALLER_HPP

#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"

namespace AML {

// Calls the primitive behind an id on argument values.
template<typename IGetPrimitiveFunction>
struct primitive_caller {
    primitive_caller(IGetPrimitiveFunction& get_primitive_function);
    value call(prim_id prim, const std::vector<const value*>& args) const;
private:
    IGetPrimitiveFunction& get_primitive_function_;
};

#define PRIMITIVE_CALLER_TEMPLATE template<typename IGetPrimitiveFunction>
#define PRIMITIVE_CALLER          primitive_caller<IGetPrimitiveFunction>

PRIMITIVE_CALLER_TEMPLATE
PRIMITIVE_CALLER::primitive_caller(IGetPrimitiveFunction& get_primitive_function)
    : get_primitive_function_(get_primitive_function) {}

PRIMITIVE_CALLER_TEMPLATE
value PRIMITIVE_CALLER::call(prim_id prim, const std::vector<const value*>& args) const {
    return get_primitive_function_.function(prim)(args);
}

#undef PRIMITIVE_CALLER
#undef PRIMITIVE_CALLER_TEMPLATE

}

#endif
