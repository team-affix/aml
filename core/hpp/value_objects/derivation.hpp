#ifndef DERIVATION_HPP
#define DERIVATION_HPP

#include <compare>
#include <variant>
#include <vector>
#include "value_objects/operand.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/var_id.hpp"

namespace AML {

// How a feature of a feature graph is made: an input column, or one primitive applied to operands, each a feature or
// a constant.
struct derivation {
    struct input {
        var_id index;
        auto operator<=>(const input&) const = default;
    };
    struct application {
        prim_id prim;
        std::vector<operand> args;
        auto operator<=>(const application&) const = default;
    };
    std::variant<input, application> content;
    std::strong_ordering operator<=>(const derivation&) const = default;
};

}

#endif
