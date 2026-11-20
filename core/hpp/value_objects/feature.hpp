#ifndef FEATURE_HPP
#define FEATURE_HPP

#include <compare>
#include <variant>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"
#include "value_objects/var_id.hpp"

namespace AML {

// An expression over the variables in scope: a variable, a primitive application, or a constant. A feature owns its
// arguments, so it is a tree; the node types nest inside it so that `app` can hold features without a forward
// declaration. The recursive comparisons name their result type, since an `auto` one cannot be deduced while
// `feature` is still incomplete.
struct feature {
    struct var {
        var_id index;
        auto operator<=>(const var&) const = default;
    };
    struct app {
        prim_id prim;
        std::vector<feature> args;
        std::strong_ordering operator<=>(const app&) const = default;
    };
    struct constant {
        const value* literal;
        auto operator<=>(const constant&) const = default;
    };
    std::variant<var, app, constant> content;
    std::strong_ordering operator<=>(const feature&) const = default;
};

}

#endif
