#ifndef ARGUMENT_OPTION_FINDER_HPP
#define ARGUMENT_OPTION_FINDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

// Finds the operands that can fill a primitive's next argument after a prefix of chosen operands and still leave an
// application witness. Operands are the session's constants followed by its features, in that order.
template<typename IGetConstants, typename IGetFeatureCandidates, typename IDetectWitness>
struct argument_option_finder {
    argument_option_finder(
        IGetConstants& get_constants,
        IGetFeatureCandidates& get_feature_candidates,
        IDetectWitness& detect_witness);
    std::vector<size_t> find(
        prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const;
private:
    IGetConstants& get_constants_;
    IGetFeatureCandidates& get_feature_candidates_;
    IDetectWitness& detect_witness_;
};

#define ARGUMENT_OPTION_FINDER_TEMPLATE template<typename IGC, typename IGFC, typename IDW>
#define ARGUMENT_OPTION_FINDER          argument_option_finder<IGC, IGFC, IDW>

ARGUMENT_OPTION_FINDER_TEMPLATE
ARGUMENT_OPTION_FINDER::argument_option_finder(
    IGC& get_constants, IGFC& get_feature_candidates, IDW& detect_witness)
    : get_constants_(get_constants)
    , get_feature_candidates_(get_feature_candidates)
    , detect_witness_(detect_witness) {}

ARGUMENT_OPTION_FINDER_TEMPLATE
std::vector<size_t> ARGUMENT_OPTION_FINDER::find(
    prim_id prim, const std::vector<type_id>& arg_types, const std::vector<size_t>& prefix) const {
    DEBUG_ASSERT(prefix.size() < arg_types.size());
    const std::vector<constant_operand>& constants = get_constants_.constants();
    const std::vector<candidate>& features = get_feature_candidates_.candidates();
    const type_id next_type = arg_types[prefix.size()];

    std::vector<size_t> options;
    std::vector<size_t> extended = prefix;
    extended.push_back(0);
    for (size_t operand = 0; operand < constants.size() + features.size(); ++operand) {
        const bool is_constant = operand < constants.size();
        const type_id operand_type =
            is_constant ? constants[operand].type : features[operand - constants.size()].observed.type;
        if (operand_type != next_type)
            continue;
        extended.back() = operand;
        if (!detect_witness_.detect(prim, arg_types, extended))
            continue;
        options.push_back(operand);
    }
    return options;
}

#undef ARGUMENT_OPTION_FINDER
#undef ARGUMENT_OPTION_FINDER_TEMPLATE

#endif
