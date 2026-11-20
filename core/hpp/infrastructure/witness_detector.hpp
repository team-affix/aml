#ifndef WITNESS_DETECTOR_HPP
#define WITNESS_DETECTOR_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

namespace AML {

// Detects an application witness: an argument tuple for a primitive that extends a prefix of operands, type-checks,
// holds at least one feature (so is heterogeneous), and has not been tried. Tuples are counted per argument type,
// never listed.
template<
    typename ICheckFeature,
    typename ICountOperandsOfType,
    typename ICountConstantsOfType,
    typename ICountTriedApplications>
struct witness_detector {
    witness_detector(
        ICheckFeature& check_feature,
        ICountOperandsOfType& count_operands_of_type,
        ICountConstantsOfType& count_constants_of_type,
        ICountTriedApplications& count_tried_applications);
    bool detect(prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const;
private:
    ICheckFeature& check_feature_;
    ICountOperandsOfType& count_operands_of_type_;
    ICountConstantsOfType& count_constants_of_type_;
    ICountTriedApplications& count_tried_applications_;
};

#define WITNESS_DETECTOR_TEMPLATE template<typename ICF, typename ICOT, typename ICCT, typename ICTA>
#define WITNESS_DETECTOR          witness_detector<ICF, ICOT, ICCT, ICTA>

WITNESS_DETECTOR_TEMPLATE
WITNESS_DETECTOR::witness_detector(
    ICF& check_feature, ICOT& count_operands_of_type, ICCT& count_constants_of_type, ICTA& count_tried_applications)
    : check_feature_(check_feature)
    , count_operands_of_type_(count_operands_of_type)
    , count_constants_of_type_(count_constants_of_type)
    , count_tried_applications_(count_tried_applications) {}

WITNESS_DETECTOR_TEMPLATE
bool WITNESS_DETECTOR::detect(
    prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const {
    DEBUG_ASSERT(prefix.size() <= arg_types.size());
    bool prefix_heterogeneous = false;
    for (const size_t operand : prefix)
        prefix_heterogeneous = prefix_heterogeneous || check_feature_.is_feature(operand);

    size_t completions = 1;
    size_t constant_completions = 1;
    for (size_t position = prefix.size(); position < arg_types.size(); ++position) {
        completions *= count_operands_of_type_.count_of_type(arg_types[position]);
        constant_completions *= count_constants_of_type_.constant_count_of_type(arg_types[position]);
    }

    const size_t useful = prefix_heterogeneous ? completions : completions - constant_completions;
    return useful > count_tried_applications_.count_with_prefix(prim, prefix);
}

#undef WITNESS_DETECTOR
#undef WITNESS_DETECTOR_TEMPLATE

}

#endif
