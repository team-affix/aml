#ifndef TYPE_DEF_HPP
#define TYPE_DEF_HPP

#include <vector>
#include "value_objects/ctor_def.hpp"

// A user type: a monomorphic algebraic data type given by its constructors, in tag order.
struct type_def {
    std::vector<ctor_def> ctors;
    auto operator<=>(const type_def&) const = default;
};

#endif
