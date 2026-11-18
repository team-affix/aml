#ifndef SCRUTINEE_CHOOSER_HPP
#define SCRUTINEE_CHOOSER_HPP

#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "debug_assert.hpp"

// Chooses the scrutinee to partition a bin on: runs a synthesis session over the bin and chooses one of its features.
// The scrutinee is returned as its own value, so it outlives the session.
template<typename ISynthesizeFeatures, typename IChooseFeature>
struct scrutinee_chooser {
    scrutinee_chooser(ISynthesizeFeatures& synthesize_features, IChooseFeature& choose_feature);
    candidate choose(const bin& data);
private:
    ISynthesizeFeatures& synthesize_features_;
    IChooseFeature& choose_feature_;
};

#define SCRUTINEE_CHOOSER_TEMPLATE template<typename ISynthesizeFeatures, typename IChooseFeature>
#define SCRUTINEE_CHOOSER          scrutinee_chooser<ISynthesizeFeatures, IChooseFeature>

SCRUTINEE_CHOOSER_TEMPLATE
SCRUTINEE_CHOOSER::scrutinee_chooser(ISynthesizeFeatures& synthesize_features, IChooseFeature& choose_feature)
    : synthesize_features_(synthesize_features)
    , choose_feature_(choose_feature) {}

SCRUTINEE_CHOOSER_TEMPLATE
candidate SCRUTINEE_CHOOSER::choose(const bin& data) {
    const std::vector<candidate>& features = synthesize_features_.synthesize(data);
    DEBUG_ASSERT(!features.empty());
    return features.at(choose_feature_.choose(features.size()));
}

#undef SCRUTINEE_CHOOSER
#undef SCRUTINEE_CHOOSER_TEMPLATE

#endif
