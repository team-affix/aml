#ifndef PRIMITIVE_OPTION_FINDER_HPP
#define PRIMITIVE_OPTION_FINDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_type.hpp"

namespace AML {

// Finds the primitives that have an application witness, in id order.
template<typename IGetPrimitiveCount, typename IGetPrimitiveType, typename IDetectWitness>
struct primitive_option_finder {
    primitive_option_finder(
        IGetPrimitiveCount& get_primitive_count,
        IGetPrimitiveType& get_primitive_type,
        IDetectWitness& detect_witness);
    std::vector<prim_id> find() const;
private:
    IGetPrimitiveCount& get_primitive_count_;
    IGetPrimitiveType& get_primitive_type_;
    IDetectWitness& detect_witness_;
};

#define PRIMITIVE_OPTION_FINDER_TEMPLATE template<typename IGPC, typename IGPT, typename IDW>
#define PRIMITIVE_OPTION_FINDER          primitive_option_finder<IGPC, IGPT, IDW>

PRIMITIVE_OPTION_FINDER_TEMPLATE
PRIMITIVE_OPTION_FINDER::primitive_option_finder(
    IGPC& get_primitive_count, IGPT& get_primitive_type, IDW& detect_witness)
    : get_primitive_count_(get_primitive_count)
    , get_primitive_type_(get_primitive_type)
    , detect_witness_(detect_witness) {}

PRIMITIVE_OPTION_FINDER_TEMPLATE
std::vector<prim_id> PRIMITIVE_OPTION_FINDER::find() const {
    std::vector<prim_id> options;
    for (prim_id prim = 0; prim < get_primitive_count_.count(); ++prim) {
        const primitive_type& type = get_primitive_type_.type(prim);
        if (!detect_witness_.detect(prim, type.arg_types, {}))
            continue;
        options.push_back(prim);
    }
    return options;
}

#undef PRIMITIVE_OPTION_FINDER
#undef PRIMITIVE_OPTION_FINDER_TEMPLATE

}

#endif
