#ifndef FEATURE_RECENCY_MEASURER_HPP
#define FEATURE_RECENCY_MEASURER_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// Measures how recent a session's feature is: a variable is 0, being there from the session's start, and a
// synthesized feature is its creation time, so a larger value is a more recently created feature.
template<typename IGetFeatures, typename IGetCreationTime>
struct feature_recency_measurer {
    feature_recency_measurer(IGetFeatures& get_features, IGetCreationTime& get_creation_time);
    size_t measure(size_t position) const;
private:
    IGetFeatures& get_features_;
    IGetCreationTime& get_creation_time_;
};

#define FEATURE_RECENCY_MEASURER_TEMPLATE template<typename IGetFeatures, typename IGetCreationTime>
#define FEATURE_RECENCY_MEASURER          feature_recency_measurer<IGetFeatures, IGetCreationTime>

FEATURE_RECENCY_MEASURER_TEMPLATE
FEATURE_RECENCY_MEASURER::feature_recency_measurer(IGetFeatures& get_features, IGetCreationTime& get_creation_time)
    : get_features_(get_features)
    , get_creation_time_(get_creation_time) {}

FEATURE_RECENCY_MEASURER_TEMPLATE
size_t FEATURE_RECENCY_MEASURER::measure(size_t position) const {
    const std::vector<candidate>& features = get_features_.candidates();
    if (std::holds_alternative<feature::var>(features.at(position).expression.content))
        return 0;
    return get_creation_time_.creation_time(position);
}

#undef FEATURE_RECENCY_MEASURER
#undef FEATURE_RECENCY_MEASURER_TEMPLATE

}

#endif
