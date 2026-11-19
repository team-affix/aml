#ifndef PRIMITIVE_STORE_HPP
#define PRIMITIVE_STORE_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive.hpp"
#include "value_objects/primitive_type.hpp"
#include "value_objects/typed_primitive.hpp"

// Stores the primitives a solver was given, each with its function type. A primitive's id is its position.
struct primitive_store {
    primitive_store(const std::vector<typed_primitive>& primitives);
    size_t count() const;
    const primitive_type& type(prim_id prim) const;
    const primitive& function(prim_id prim) const;
private:
    std::vector<typed_primitive> primitives_;
};

#endif
