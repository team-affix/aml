#ifndef VALUE_HPP
#define VALUE_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"

// A constructor term. Values are interned, so two values are equal exactly when their pointers are.
struct value {
    ctor_id ctor;
    std::vector<const value*> args;
    auto operator<=>(const value&) const = default;
};

#endif
