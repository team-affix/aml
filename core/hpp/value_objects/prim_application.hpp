#ifndef PRIM_APPLICATION_HPP
#define PRIM_APPLICATION_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"

namespace AML {

// A primitive applied to a tuple of operand indices: the session's constants first, then its features.
struct prim_application {
    prim_id prim;
    std::vector<size_t> args;
    auto operator<=>(const prim_application&) const = default;
};

}

#endif
