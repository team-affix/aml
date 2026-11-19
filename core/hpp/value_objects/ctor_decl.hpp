#ifndef CTOR_DECL_HPP
#define CTOR_DECL_HPP

#include <cstddef>
#include <vector>
#include "value_objects/type_id.hpp"

// A constructor of a monomorphic algebraic data type. `tag` is its position among its type's constructors.
struct ctor_decl {
    type_id type;
    size_t tag;
    std::vector<type_id> field_types;
    auto operator<=>(const ctor_decl&) const = default;
};

#endif
