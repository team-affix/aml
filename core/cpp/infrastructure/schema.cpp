#include "infrastructure/schema.hpp"
#include "debug_assert.hpp"

schema::schema()
    : types_() {}

type_id schema::declare_type() {
    types_.push_back(type_def{.ctors = {}});
    return static_cast<type_id>(types_.size() - 1);
}

size_t schema::declare_ctor(type_id type, const std::vector<type_id>& arg_types) {
    DEBUG_ASSERT(type < types_.size());
    for (const type_id arg_type : arg_types)
        DEBUG_ASSERT(arg_type < types_.size());
    std::vector<ctor_def>& ctors = types_[type].ctors;
    ctors.push_back(ctor_def{.arg_types = arg_types});
    return ctors.size() - 1;
}

size_t schema::type_count() const {
    return types_.size();
}

const type_def& schema::def(type_id type) const {
    return types_.at(type);
}
