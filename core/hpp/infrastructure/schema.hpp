#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <cstddef>
#include <vector>
#include "value_objects/ctor_def.hpp"
#include "value_objects/type_def.hpp"
#include "value_objects/type_id.hpp"

// The user's types, and the authority on type ids and constructor tags: every id and tag comes from here. Declaring a
// type gives its id; declaring a constructor of a declared type gives its tag, the constructors of a type numbered in
// the order declared. A constructor's argument types may be any declared type, the type itself included, so declaring
// types before their constructors allows recursive and mutually recursive types.
struct schema {
    schema();
    type_id declare_type();
    size_t declare_ctor(type_id type, const std::vector<type_id>& arg_types);
    size_t type_count() const;
    const type_def& def(type_id type) const;
private:
    std::vector<type_def> types_;
};

#endif
