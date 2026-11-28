#ifndef CONSTANT_DERIVATION_HPP
#define CONSTANT_DERIVATION_HPP

#include <compare>
#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// How a constant of a constant graph is made: an offered constant (by its position among the offered constants), or
// one primitive applied to other constants.
struct constant_derivation {
    struct atom {
        size_t index;
        auto operator<=>(const atom&) const = default;
    };
    struct application {
        prim_id prim;
        std::vector<constant_id> args;
        auto operator<=>(const application&) const = default;
    };
    std::variant<atom, application> content;
    std::strong_ordering operator<=>(const constant_derivation&) const = default;
};

}

#endif
