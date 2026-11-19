#include "infrastructure/schema.hpp"
#include "debug_assert.hpp"

schema::schema(const std::vector<type_def>& types)
    : types_(types) {
    for (const type_def& type : types_) {
        for (const ctor_def& ctor : type.ctors) {
            for (const type_id arg_type : ctor.arg_types)
                DEBUG_ASSERT(arg_type < types_.size());
        }
    }
}

size_t schema::type_count() const {
    return types_.size();
}

const type_def& schema::def(type_id type) const {
    return types_.at(type);
}
