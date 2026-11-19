#include "infrastructure/schema.hpp"
#include "debug_assert.hpp"

schema::schema()
    : type_ctors_()
    , ctor_decls_() {}

type_id schema::declare_type() {
    type_ctors_.emplace_back();
    return static_cast<type_id>(type_ctors_.size() - 1);
}

ctor_id schema::declare_ctor(type_id type, const std::vector<type_id>& field_types) {
    DEBUG_ASSERT(type < type_ctors_.size());
    const ctor_id ctor = static_cast<ctor_id>(ctor_decls_.size());
    std::vector<ctor_id>& siblings = type_ctors_[type];
    ctor_decls_.push_back(ctor_decl{.type = type, .tag = siblings.size(), .field_types = field_types});
    siblings.push_back(ctor);
    return ctor;
}

const ctor_decl& schema::decl(ctor_id ctor) const {
    return ctor_decls_.at(ctor);
}

const std::vector<ctor_id>& schema::ctors(type_id type) const {
    return type_ctors_.at(type);
}

size_t schema::type_count() const {
    return type_ctors_.size();
}
