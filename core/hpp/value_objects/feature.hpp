#ifndef FEATURE_HPP
#define FEATURE_HPP

#include <variant>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"
#include "value_objects/var_id.hpp"

// An expression over the variables in scope: a variable, a primitive application, or a constant.
struct feature {
    struct var {
        var_id index;
        auto operator<=>(const var&) const = default;
    };
    struct app {
        prim_id prim;
        std::vector<const feature*> args;
        auto operator<=>(const app&) const = default;
    };
    struct constant {
        const value* literal;
        auto operator<=>(const constant&) const = default;
    };
    std::variant<var, app, constant> content;
    auto operator<=>(const feature&) const = default;
};

#endif
