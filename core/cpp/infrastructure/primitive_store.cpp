#include "infrastructure/primitive_store.hpp"

namespace AML {

primitive_store::primitive_store(const std::vector<typed_primitive>& primitives)
    : primitives_(primitives) {}

size_t primitive_store::count() const {
    return primitives_.size();
}

const primitive_type& primitive_store::type(prim_id prim) const {
    return primitives_.at(prim).type;
}

const primitive& primitive_store::function(prim_id prim) const {
    return primitives_.at(prim).apply;
}

}
