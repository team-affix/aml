#ifndef INHERITING_SYNTHESIS_STARTER_HPP
#define INHERITING_SYNTHESIS_STARTER_HPP

#include <cstddef>
#include <utility>
#include "value_objects/bin_view.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// Starts a synthesis session as the starter it wraps does, then adds the features the bin inherits from the matches
// above it, each evaluated over the bin's rows, after the variables. An inherited feature that is uniform over the bin
// is left out, as every session feature is heterogeneous. Inherited features cost their full size, as if rebuilt.
template<
    typename IStartSession,
    typename IGetInheritedFeatures,
    typename IEvaluateFeatureColumn,
    typename IFindUniformValue,
    typename ISizeFeature,
    typename ITryInsertFeatureCandidate>
struct inheriting_synthesis_starter {
    inheriting_synthesis_starter(
        IStartSession& start_session,
        IGetInheritedFeatures& get_inherited_features,
        IEvaluateFeatureColumn& evaluate_feature_column,
        IFindUniformValue& find_uniform_value,
        ISizeFeature& size_feature,
        ITryInsertFeatureCandidate& try_insert_feature_candidate);
    void start(const bin_view& data);
private:
    IStartSession& start_session_;
    IGetInheritedFeatures& get_inherited_features_;
    IEvaluateFeatureColumn& evaluate_feature_column_;
    IFindUniformValue& find_uniform_value_;
    ISizeFeature& size_feature_;
    ITryInsertFeatureCandidate& try_insert_feature_candidate_;
};

#define INHERITING_SYNTHESIS_STARTER_TEMPLATE \
    template<typename ISS, typename IGIF, typename IEFC, typename IFUV, typename ISF, typename ITIFC>
#define INHERITING_SYNTHESIS_STARTER inheriting_synthesis_starter<ISS, IGIF, IEFC, IFUV, ISF, ITIFC>

INHERITING_SYNTHESIS_STARTER_TEMPLATE
INHERITING_SYNTHESIS_STARTER::inheriting_synthesis_starter(
    ISS& start_session,
    IGIF& get_inherited_features,
    IEFC& evaluate_feature_column,
    IFUV& find_uniform_value,
    ISF& size_feature,
    ITIFC& try_insert_feature_candidate)
    : start_session_(start_session)
    , get_inherited_features_(get_inherited_features)
    , evaluate_feature_column_(evaluate_feature_column)
    , find_uniform_value_(find_uniform_value)
    , size_feature_(size_feature)
    , try_insert_feature_candidate_(try_insert_feature_candidate) {}

INHERITING_SYNTHESIS_STARTER_TEMPLATE
void INHERITING_SYNTHESIS_STARTER::start(const bin_view& data) {
    start_session_.start(data);
    for (const feature& inherited : get_inherited_features_.inherited(data.labels.rows.get())) {
        column_view observed = evaluate_feature_column_.evaluate(inherited, data);
        if (find_uniform_value_.find(observed).has_value())
            continue;
        const size_t cost = size_feature_.size(inherited);
        try_insert_feature_candidate_.try_insert(
            candidate{.expression = inherited, .cost = cost, .observed = std::move(observed)});
    }
}

#undef INHERITING_SYNTHESIS_STARTER
#undef INHERITING_SYNTHESIS_STARTER_TEMPLATE

}

#endif
