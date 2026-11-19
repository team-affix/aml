#ifndef TYPED_CONSTANT_HPP
#define TYPED_CONSTANT_HPP

#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"

// A constant the user offers synthesis as a primitive argument, with its type.
struct typed_constant {
    type_id type;
    const value* literal;
    auto operator<=>(const typed_constant&) const = default;
};

#endif
