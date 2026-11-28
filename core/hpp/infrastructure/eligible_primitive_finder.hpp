#ifndef ELIGIBLE_PRIMITIVE_FINDER_HPP
#define ELIGIBLE_PRIMITIVE_FINDER_HPP

#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Finds the primitives a pool of features can feed: those taking an argument of a type the pool holds a feature of,
// each once, in increasing id. Only they can be applied with a feature among their operands.
template<typename IGetPrimitivesOfArgumentType>
struct eligible_primitive_finder {
    eligible_primitive_finder(IGetPrimitivesOfArgumentType& get_primitives_of_argument_type);
    std::vector<prim_id> find(const std::unordered_map<std::type_index, std::vector<feature_id>>& pool) const;
private:
    IGetPrimitivesOfArgumentType& get_primitives_of_argument_type_;
};

#define ELIGIBLE_PRIMITIVE_FINDER_TEMPLATE template<typename IGetPrimitivesOfArgumentType>
#define ELIGIBLE_PRIMITIVE_FINDER          eligible_primitive_finder<IGetPrimitivesOfArgumentType>

ELIGIBLE_PRIMITIVE_FINDER_TEMPLATE
ELIGIBLE_PRIMITIVE_FINDER::eligible_primitive_finder(IGetPrimitivesOfArgumentType& get_primitives_of_argument_type)
    : get_primitives_of_argument_type_(get_primitives_of_argument_type) {}

ELIGIBLE_PRIMITIVE_FINDER_TEMPLATE
std::vector<prim_id> ELIGIBLE_PRIMITIVE_FINDER::find(
    const std::unordered_map<std::type_index, std::vector<feature_id>>& pool) const {
    std::vector<prim_id> eligible;
    for (const auto& [type, features] : pool) {
        if (features.empty())
            continue;
        const std::vector<prim_id>& takers = get_primitives_of_argument_type_.of_type(type);
        eligible.insert(eligible.end(), takers.begin(), takers.end());
    }
    std::sort(eligible.begin(), eligible.end());
    eligible.erase(std::unique(eligible.begin(), eligible.end()), eligible.end());
    return eligible;
}

#undef ELIGIBLE_PRIMITIVE_FINDER
#undef ELIGIBLE_PRIMITIVE_FINDER_TEMPLATE

}

#endif
