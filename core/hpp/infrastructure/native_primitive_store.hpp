#ifndef NATIVE_PRIMITIVE_STORE_HPP
#define NATIVE_PRIMITIVE_STORE_HPP

#include <cstddef>
#include <vector>
#include "value_objects/native_primitive.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Stores the native primitives a column solver was given. A primitive's id is its position.
struct native_primitive_store {
    native_primitive_store(const std::vector<native_primitive>& primitives);
    size_t count() const;
    const native_primitive& primitive(prim_id prim) const;
private:
    std::vector<native_primitive> primitives_;
};

}

#endif
