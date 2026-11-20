#ifndef MODEL_NODE_HPP
#define MODEL_NODE_HPP

#include <compare>
#include <variant>
#include <vector>
#include "value_objects/feature.hpp"

namespace AML {

// A node of a model, which owns its branches, so a model is a tree. `match` is an exhaustive one-layer case on
// `scrutinee`: `branches[t]` handles the constructor with tag t and sees that constructor's arguments bound as the next
// variables in scope. `leaf` returns its output feature evaluated in scope: a constant label, a variable, or any
// synthesized feature. `empty` saw no data and returns nothing. The node types nest inside `model_node` so that
// `match` can hold nodes without a forward declaration; the recursive comparisons name their result type, since an
// `auto` one cannot be deduced while `model_node` is still incomplete.
struct model_node {
    struct match {
        feature scrutinee;
        std::vector<model_node> branches;
        std::strong_ordering operator<=>(const match&) const = default;
    };
    struct leaf {
        feature output;
        auto operator<=>(const leaf&) const = default;
    };
    struct empty {
        auto operator<=>(const empty&) const = default;
    };
    std::variant<match, leaf, empty> content;
    std::strong_ordering operator<=>(const model_node&) const = default;
};

}

#endif
