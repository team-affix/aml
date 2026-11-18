#ifndef WITNESS_DETECTOR_HPP
#define WITNESS_DETECTOR_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

// Detects an application witness: an argument tuple for a primitive that extends a prefix of operand indices,
// type-checks, holds at least one heterogeneous feature, and has not been tried. Operands are the session's constants
// followed by its features; every feature is heterogeneous and every constant is not. Tuples are counted, not listed.
template<typename IGetConstants, typename IGetFeatureCandidates, typename ICountTriedApplications>
struct witness_detector {
    witness_detector(
        IGetConstants& get_constants,
        IGetFeatureCandidates& get_feature_candidates,
        ICountTriedApplications& count_tried_applications);
    bool detect(prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const;
private:
    IGetConstants& get_constants_;
    IGetFeatureCandidates& get_feature_candidates_;
    ICountTriedApplications& count_tried_applications_;
};

#define WITNESS_DETECTOR_TEMPLATE template<typename IGC, typename IGFC, typename ICTA>
#define WITNESS_DETECTOR          witness_detector<IGC, IGFC, ICTA>

WITNESS_DETECTOR_TEMPLATE
WITNESS_DETECTOR::witness_detector(
    IGC& get_constants, IGFC& get_feature_candidates, ICTA& count_tried_applications)
    : get_constants_(get_constants)
    , get_feature_candidates_(get_feature_candidates)
    , count_tried_applications_(count_tried_applications) {}

WITNESS_DETECTOR_TEMPLATE
bool WITNESS_DETECTOR::detect(
    prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const {
    DEBUG_ASSERT(prefix.size() <= arg_types.size());
    const std::vector<constant_operand>& constants = get_constants_.constants();
    const std::vector<candidate>& features = get_feature_candidates_.candidates();

    bool prefix_heterogeneous = false;
    for (const size_t operand : prefix)
        prefix_heterogeneous = prefix_heterogeneous || operand >= constants.size();

    size_t completions = 1;
    size_t constant_completions = 1;
    for (size_t position = prefix.size(); position < arg_types.size(); ++position) {
        size_t constant_count = 0;
        for (const constant_operand& constant : constants) {
            if (constant.type != arg_types[position])
                continue;
            ++constant_count;
        }
        size_t feature_count = 0;
        for (const candidate& held : features) {
            if (held.observed.type != arg_types[position])
                continue;
            ++feature_count;
        }
        completions *= constant_count + feature_count;
        constant_completions *= constant_count;
    }

    const size_t useful = prefix_heterogeneous ? completions : completions - constant_completions;
    return useful > count_tried_applications_.count_with_prefix(prim, prefix);
}

#undef WITNESS_DETECTOR
#undef WITNESS_DETECTOR_TEMPLATE

#endif
