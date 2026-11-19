#ifndef CTOR_REF_HPP
#define CTOR_REF_HPP

#include <cstddef>
#include "value_objects/type_id.hpp"

// Names a constructor by its type and its position (tag) among that type's constructors.
struct ctor_ref {
    type_id type;
    size_t tag;
    auto operator<=>(const ctor_ref&) const = default;
};

#endif
