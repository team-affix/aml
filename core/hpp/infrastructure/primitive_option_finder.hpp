#ifndef PRIMITIVE_OPTION_FINDER_HPP
#define PRIMITIVE_OPTION_FINDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"

// Finds the primitives that have an application witness, in id order.
template<typename IGetPrimitiveCount, typename IGetPrimitiveSignature, typename IDetectWitness>
struct primitive_option_finder {
    primitive_option_finder(
        IGetPrimitiveCount& get_primitive_count,
        IGetPrimitiveSignature& get_primitive_signature,
        IDetectWitness& detect_witness);
    std::vector<prim_id> find() const;
private:
    IGetPrimitiveCount& get_primitive_count_;
    IGetPrimitiveSignature& get_primitive_signature_;
    IDetectWitness& detect_witness_;
};

#define PRIMITIVE_OPTION_FINDER_TEMPLATE template<typename IGPC, typename IGPS, typename IDW>
#define PRIMITIVE_OPTION_FINDER          primitive_option_finder<IGPC, IGPS, IDW>

PRIMITIVE_OPTION_FINDER_TEMPLATE
PRIMITIVE_OPTION_FINDER::primitive_option_finder(
    IGPC& get_primitive_count, IGPS& get_primitive_signature, IDW& detect_witness)
    : get_primitive_count_(get_primitive_count)
    , get_primitive_signature_(get_primitive_signature)
    , detect_witness_(detect_witness) {}

PRIMITIVE_OPTION_FINDER_TEMPLATE
std::vector<prim_id> PRIMITIVE_OPTION_FINDER::find() const {
    std::vector<prim_id> options;
    for (prim_id prim = 0; prim < get_primitive_count_.count(); ++prim) {
        const primitive_signature signature = get_primitive_signature_.signature(prim);
        if (!detect_witness_.detect(prim, signature.arg_types, {}))
            continue;
        options.push_back(prim);
    }
    return options;
}

#undef PRIMITIVE_OPTION_FINDER
#undef PRIMITIVE_OPTION_FINDER_TEMPLATE

#endif
