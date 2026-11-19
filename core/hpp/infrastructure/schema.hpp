#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <cstddef>
#include <vector>
#include "value_objects/ctor_decl.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/type_id.hpp"

// Stores the monomorphic algebraic data types: each type's constructors and each constructor's field types.
struct schema {
    schema();
    type_id declare_type();
    ctor_id declare_ctor(type_id type, const std::vector<type_id>& field_types);
    const ctor_decl& decl(ctor_id ctor) const;
    const std::vector<ctor_id>& ctors(type_id type) const;
    size_t type_count() const;
private:
    std::vector<std::vector<ctor_id>> type_ctors_;
    std::vector<ctor_decl> ctor_decls_;
};

#endif
