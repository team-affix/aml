#ifndef FILLABLE_PRIMITIVE_FINDER_HPP
#define FILLABLE_PRIMITIVE_FINDER_HPP

#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Finds the eligible primitives every argument of which has a candidate, a pool feature or a constant of its type, in
// the order given. Only they can be applied over the pool.
template<typename IGetNativePrimitive, typename IGetConstantsOfType>
struct fillable_primitive_finder {
    fillable_primitive_finder(
        IGetNativePrimitive& get_native_primitive, IGetConstantsOfType& get_constants_of_type);
    std::vector<prim_id> find(
        const std::unordered_map<std::type_index, std::vector<feature_id>>& pool,
        const std::vector<prim_id>& eligible) const;
private:
    IGetNativePrimitive& get_native_primitive_;
    IGetConstantsOfType& get_constants_of_type_;
};

#define FILLABLE_PRIMITIVE_FINDER_TEMPLATE template<typename IGetNativePrimitive, typename IGetConstantsOfType>
#define FILLABLE_PRIMITIVE_FINDER          fillable_primitive_finder<IGetNativePrimitive, IGetConstantsOfType>

FILLABLE_PRIMITIVE_FINDER_TEMPLATE
FILLABLE_PRIMITIVE_FINDER::fillable_primitive_finder(
    IGetNativePrimitive& get_native_primitive, IGetConstantsOfType& get_constants_of_type)
    : get_native_primitive_(get_native_primitive)
    , get_constants_of_type_(get_constants_of_type) {}

FILLABLE_PRIMITIVE_FINDER_TEMPLATE
std::vector<prim_id> FILLABLE_PRIMITIVE_FINDER::find(
    const std::unordered_map<std::type_index, std::vector<feature_id>>& pool,
    const std::vector<prim_id>& eligible) const {
    std::vector<prim_id> fillable;
    for (const prim_id prim : eligible) {
        bool filled = true;
        for (const std::type_index& type : get_native_primitive_.primitive(prim).arg_types) {
            const auto pooled = pool.find(type);
            const bool has_feature = pooled != pool.end() && !pooled->second.empty();
            filled = filled && (has_feature || !get_constants_of_type_.of_type(type).empty());
        }
        if (!filled)
            continue;
        fillable.push_back(prim);
    }
    return fillable;
}

#undef FILLABLE_PRIMITIVE_FINDER
#undef FILLABLE_PRIMITIVE_FINDER_TEMPLATE

}

#endif
