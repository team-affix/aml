#include "infrastructure/native_primitive_store.hpp"

namespace AML {

native_primitive_store::native_primitive_store(const std::vector<native_primitive>& primitives)
    : primitives_(primitives) {}

size_t native_primitive_store::count() const {
    return primitives_.size();
}

const native_primitive& native_primitive_store::primitive(prim_id prim) const {
    return primitives_.at(prim);
}

}
