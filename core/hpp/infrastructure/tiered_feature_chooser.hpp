#ifndef TIERED_FEATURE_CHOOSER_HPP
#define TIERED_FEATURE_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses one of a synthesis session's features, read from the session's store, by the features' orders: the
// highest-order features are offered first, so a bin is likely to be matched on the feature built up furthest.
template<typename IGetFeatures, typename IMeasureOrder, typename IChooseOption>
struct tiered_feature_chooser {
    tiered_feature_chooser(IGetFeatures& get_features, IMeasureOrder& measure_order, IChooseOption& choose_option);
    size_t choose(size_t feature_count);
private:
    IGetFeatures& get_features_;
    IMeasureOrder& measure_order_;
    IChooseOption& choose_option_;
};

#define TIERED_FEATURE_CHOOSER_TEMPLATE template<typename IGetFeatures, typename IMeasureOrder, typename IChooseOption>
#define TIERED_FEATURE_CHOOSER          tiered_feature_chooser<IGetFeatures, IMeasureOrder, IChooseOption>

TIERED_FEATURE_CHOOSER_TEMPLATE
TIERED_FEATURE_CHOOSER::tiered_feature_chooser(
    IGetFeatures& get_features, IMeasureOrder& measure_order, IChooseOption& choose_option)
    : get_features_(get_features)
    , measure_order_(measure_order)
    , choose_option_(choose_option) {}

TIERED_FEATURE_CHOOSER_TEMPLATE
size_t TIERED_FEATURE_CHOOSER::choose(size_t feature_count) {
    const std::vector<candidate>& features = get_features_.candidates();
    DEBUG_ASSERT(feature_count == features.size());
    std::vector<size_t> orders;
    for (const candidate& entry : features)
        orders.push_back(measure_order_.measure(entry.expression));
    return choose_option_.choose(orders);
}

#undef TIERED_FEATURE_CHOOSER
#undef TIERED_FEATURE_CHOOSER_TEMPLATE

}

#endif
