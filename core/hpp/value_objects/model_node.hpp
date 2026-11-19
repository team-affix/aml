#ifndef MODEL_NODE_HPP
#define MODEL_NODE_HPP

#include <variant>
#include <vector>
#include "value_objects/feature.hpp"

// A node of a model. `match` is an exhaustive one-layer case on `scrutinee`: `branches[t]` handles the constructor
// with tag t and sees that constructor's arguments bound as the next variables in scope. `leaf` returns its output
// feature evaluated in scope: a constant label, a variable, or any synthesized feature. `empty` saw no data and
// returns nothing.
struct model_node {
    struct match {
        const feature* scrutinee;
        std::vector<const model_node*> branches;
        auto operator<=>(const match&) const = default;
    };
    struct leaf {
        const feature* output;
        auto operator<=>(const leaf&) const = default;
    };
    struct empty {
        auto operator<=>(const empty&) const = default;
    };
    std::variant<match, leaf, empty> content;
    auto operator<=>(const model_node&) const = default;
};

#endif
