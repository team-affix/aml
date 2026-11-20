#ifndef CTOR_DEF_HPP
#define CTOR_DEF_HPP

#include <vector>
#include "value_objects/type_id.hpp"

namespace AML {

// A constructor of a user type: the types of its arguments.
struct ctor_def {
    std::vector<type_id> arg_types;
    auto operator<=>(const ctor_def&) const = default;
};

}

#endif
