#ifndef PRIMITIVE_TYPE_HPP
#define PRIMITIVE_TYPE_HPP

#include <vector>
#include "value_objects/type_id.hpp"

// A primitive's function type: the user types of its arguments and of its result.
struct primitive_type {
    std::vector<type_id> arg_types;
    type_id result_type;
    auto operator<=>(const primitive_type&) const = default;
};

#endif
