#ifndef SYNTHESIS_STARTER_HPP
#define SYNTHESIS_STARTER_HPP

#include <utility>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"

namespace AML {

// Starts a synthesis session over a bin: forgets the last session's features and applications, then holds the bin's
// heterogeneous variables as the first features.
template<
    typename IClearFeatureCandidates,
    typename IClearApplications,
    typename IFindVariableCandidates,
    typename ITryInsertFeatureCandidate>
struct synthesis_starter {
    synthesis_starter(
        IClearFeatureCandidates& clear_feature_candidates,
        IClearApplications& clear_applications,
        IFindVariableCandidates& find_variable_candidates,
        ITryInsertFeatureCandidate& try_insert_feature_candidate);
    void start(const bin& data);
private:
    IClearFeatureCandidates& clear_feature_candidates_;
    IClearApplications& clear_applications_;
    IFindVariableCandidates& find_variable_candidates_;
    ITryInsertFeatureCandidate& try_insert_feature_candidate_;
};

#define SYNTHESIS_STARTER_TEMPLATE template<typename ICFC, typename ICA, typename IFVC, typename ITIFC>
#define SYNTHESIS_STARTER          synthesis_starter<ICFC, ICA, IFVC, ITIFC>

SYNTHESIS_STARTER_TEMPLATE
SYNTHESIS_STARTER::synthesis_starter(
    ICFC& clear_feature_candidates,
    ICA& clear_applications,
    IFVC& find_variable_candidates,
    ITIFC& try_insert_feature_candidate)
    : clear_feature_candidates_(clear_feature_candidates)
    , clear_applications_(clear_applications)
    , find_variable_candidates_(find_variable_candidates)
    , try_insert_feature_candidate_(try_insert_feature_candidate) {}

SYNTHESIS_STARTER_TEMPLATE
void SYNTHESIS_STARTER::start(const bin& data) {
    clear_feature_candidates_.clear();
    clear_applications_.clear();
    for (candidate& variable : find_variable_candidates_.find(data, 0))
        try_insert_feature_candidate_.try_insert(std::move(variable));
}

#undef SYNTHESIS_STARTER
#undef SYNTHESIS_STARTER_TEMPLATE

}

#endif
