#ifndef VALUE_HPP
#define VALUE_HPP

#include <compare>
#include <vector>
#include "value_objects/ctor_id.hpp"

namespace AML {

// A constructor applied to argument values: a tree that owns its arguments. `ctor` is the constructor's id within its
// type; the type itself is known from where the value sits (its column), not stored. Values compare by structure.
struct value {
    ctor_id ctor;
    std::vector<value> args;
    std::strong_ordering operator<=>(const value&) const = default;
};

}

#endif
