#ifndef VALUE_HPP
#define VALUE_HPP

#include <vector>
#include "value_objects/ctor_ref.hpp"

// A constructor applied to argument values. Values are interned, so two values are equal exactly when their pointers
// are.
struct value {
    ctor_ref ctor;
    std::vector<const value*> args;
    auto operator<=>(const value&) const = default;
};

#endif
