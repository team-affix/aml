#ifndef NAT_ENCODING_HPP
#define NAT_ENCODING_HPP

#include "value_objects/ctor_id.hpp"
#include "value_objects/type_id.hpp"

// Where the Peano naturals live in a schema.
struct nat_encoding {
    type_id type;
    ctor_id zero_ctor;
    ctor_id suc_ctor;
    auto operator<=>(const nat_encoding&) const = default;
};

#endif
