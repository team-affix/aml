#include "infrastructure/native_type_store.hpp"

namespace AML {

native_type_store::native_type_store(const std::vector<native_type>& types)
    : types_() {
    for (const native_type& entry : types)
        types_.insert_or_assign(entry.type, entry);
}

const native_type& native_type_store::type(std::type_index type) const {
    return types_.at(type);
}

}
