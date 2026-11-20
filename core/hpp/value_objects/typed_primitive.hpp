#ifndef TYPED_PRIMITIVE_HPP
#define TYPED_PRIMITIVE_HPP

#include "value_objects/primitive.hpp"
#include "value_objects/primitive_type.hpp"

namespace AML {

// A primitive with its function type. Not comparable, since a primitive is a std::function.
struct typed_primitive {
    primitive_type type;
    primitive apply;
};

}

#endif
