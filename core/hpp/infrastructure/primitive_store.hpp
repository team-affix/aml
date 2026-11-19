#ifndef PRIMITIVE_STORE_HPP
#define PRIMITIVE_STORE_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"

// Stores the native primitives a solver was given. A primitive's id is its position.
template<typename IPrimitive>
struct primitive_store {
    primitive_store(const std::vector<IPrimitive>& primitives);
    size_t count() const;
    const IPrimitive& primitive_at(prim_id prim) const;
private:
    std::vector<IPrimitive> primitives_;
};

#define PRIMITIVE_STORE_TEMPLATE template<typename IPrimitive>
#define PRIMITIVE_STORE          primitive_store<IPrimitive>

PRIMITIVE_STORE_TEMPLATE
PRIMITIVE_STORE::primitive_store(const std::vector<IPrimitive>& primitives)
    : primitives_(primitives) {}

PRIMITIVE_STORE_TEMPLATE
size_t PRIMITIVE_STORE::count() const {
    return primitives_.size();
}

PRIMITIVE_STORE_TEMPLATE
const IPrimitive& PRIMITIVE_STORE::primitive_at(prim_id prim) const {
    return primitives_.at(prim);
}

#undef PRIMITIVE_STORE
#undef PRIMITIVE_STORE_TEMPLATE

#endif
