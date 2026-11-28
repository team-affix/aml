#ifndef OPERAND_HPP
#define OPERAND_HPP

#include <compare>
#include <variant>
#include "value_objects/constant_id.hpp"
#include "value_objects/feature_id.hpp"

namespace AML {

// An argument of a feature's application: a feature of the feature graph or a constant of the constant graph.
struct operand {
    struct from_feature {
        feature_id id;
        auto operator<=>(const from_feature&) const = default;
    };
    struct from_constant {
        constant_id id;
        auto operator<=>(const from_constant&) const = default;
    };
    std::variant<from_feature, from_constant> source;
    std::strong_ordering operator<=>(const operand&) const = default;
};

}

#endif
