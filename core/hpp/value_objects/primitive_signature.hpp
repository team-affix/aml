#ifndef PRIMITIVE_SIGNATURE_HPP
#define PRIMITIVE_SIGNATURE_HPP

#include <vector>
#include "value_objects/type_id.hpp"

struct primitive_signature {
    std::vector<type_id> arg_types;
    type_id result_type;
    auto operator<=>(const primitive_signature&) const = default;
};

#endif
