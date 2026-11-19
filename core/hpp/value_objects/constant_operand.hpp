#ifndef CONSTANT_OPERAND_HPP
#define CONSTANT_OPERAND_HPP

#include "value_objects/feature.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"

// A constant available as a primitive argument during synthesis.
struct constant_operand {
    feature expression;
    const value* literal;
    type_id type;
    auto operator<=>(const constant_operand&) const = default;
};

#endif
