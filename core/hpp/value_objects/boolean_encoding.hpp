#ifndef BOOLEAN_ENCODING_HPP
#define BOOLEAN_ENCODING_HPP

#include "value_objects/ctor_id.hpp"
#include "value_objects/type_id.hpp"

// Where the booleans live in a schema.
struct boolean_encoding {
    type_id type;
    ctor_id false_ctor;
    ctor_id true_ctor;
    auto operator<=>(const boolean_encoding&) const = default;
};

#endif
