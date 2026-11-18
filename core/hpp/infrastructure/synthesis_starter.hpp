#ifndef SYNTHESIS_STARTER_HPP
#define SYNTHESIS_STARTER_HPP

#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"

// Starts a synthesis session over a bin: forgets the last session, then holds the bin's constants as operands and
// its heterogeneous variables as the first features.
template<
    typename IClearConstantCandidates,
    typename IClearFeatureCandidates,
    typename IClearApplications,
    typename IFindConstantCandidates,
    typename ITryInsertConstantCandidate,
    typename IFindVariableCandidates,
    typename ITryInsertFeatureCandidate>
struct synthesis_starter {
    synthesis_starter(
        IClearConstantCandidates& clear_constant_candidates,
        IClearFeatureCandidates& clear_feature_candidates,
        IClearApplications& clear_applications,
        IFindConstantCandidates& find_constant_candidates,
        ITryInsertConstantCandidate& try_insert_constant_candidate,
        IFindVariableCandidates& find_variable_candidates,
        ITryInsertFeatureCandidate& try_insert_feature_candidate);
    void start(const bin& data);
private:
    IClearConstantCandidates& clear_constant_candidates_;
    IClearFeatureCandidates& clear_feature_candidates_;
    IClearApplications& clear_applications_;
    IFindConstantCandidates& find_constant_candidates_;
    ITryInsertConstantCandidate& try_insert_constant_candidate_;
    IFindVariableCandidates& find_variable_candidates_;
    ITryInsertFeatureCandidate& try_insert_feature_candidate_;
};

#define SYNTHESIS_STARTER_TEMPLATE \
    template<typename ICCC, typename ICFC, typename ICA, typename IFCC, typename ITICC, typename IFVC, typename ITIFC>
#define SYNTHESIS_STARTER synthesis_starter<ICCC, ICFC, ICA, IFCC, ITICC, IFVC, ITIFC>

SYNTHESIS_STARTER_TEMPLATE
SYNTHESIS_STARTER::synthesis_starter(
    ICCC& clear_constant_candidates,
    ICFC& clear_feature_candidates,
    ICA& clear_applications,
    IFCC& find_constant_candidates,
    ITICC& try_insert_constant_candidate,
    IFVC& find_variable_candidates,
    ITIFC& try_insert_feature_candidate)
    : clear_constant_candidates_(clear_constant_candidates)
    , clear_feature_candidates_(clear_feature_candidates)
    , clear_applications_(clear_applications)
    , find_constant_candidates_(find_constant_candidates)
    , try_insert_constant_candidate_(try_insert_constant_candidate)
    , find_variable_candidates_(find_variable_candidates)
    , try_insert_feature_candidate_(try_insert_feature_candidate) {}

SYNTHESIS_STARTER_TEMPLATE
void SYNTHESIS_STARTER::start(const bin& data) {
    clear_constant_candidates_.clear();
    clear_feature_candidates_.clear();
    clear_applications_.clear();
    for (const candidate& constant : find_constant_candidates_.find(data))
        try_insert_constant_candidate_.try_insert(constant);
    for (const candidate& variable : find_variable_candidates_.find(data, 0))
        try_insert_feature_candidate_.try_insert(variable);
}

#undef SYNTHESIS_STARTER
#undef SYNTHESIS_STARTER_TEMPLATE

#endif
