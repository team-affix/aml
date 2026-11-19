#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <cstddef>
#include <vector>
#include "value_objects/type_def.hpp"
#include "value_objects/type_id.hpp"

// Stores the user's types, all given at once. A type's id is its position, so a constructor's argument types may
// name any of the types, the one being defined included.
struct schema {
    schema(const std::vector<type_def>& types);
    size_t type_count() const;
    const type_def& def(type_id type) const;
private:
    std::vector<type_def> types_;
};

#endif
