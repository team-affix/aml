#ifndef OPERAND_RECENCY_MEASURER_HPP
#define OPERAND_RECENCY_MEASURER_HPP

#include <cstddef>
#include <optional>

namespace AML {

// Measures how recent an operand is: a constant is 0, being there from the session's start, and a feature is as
// recent as its feature is.
template<typename IFindFeaturePosition, typename IMeasureFeatureRecency>
struct operand_recency_measurer {
    operand_recency_measurer(
        IFindFeaturePosition& find_feature_position, IMeasureFeatureRecency& measure_feature_recency);
    size_t measure(size_t operand) const;
private:
    IFindFeaturePosition& find_feature_position_;
    IMeasureFeatureRecency& measure_feature_recency_;
};

#define OPERAND_RECENCY_MEASURER_TEMPLATE template<typename IFindFeaturePosition, typename IMeasureFeatureRecency>
#define OPERAND_RECENCY_MEASURER          operand_recency_measurer<IFindFeaturePosition, IMeasureFeatureRecency>

OPERAND_RECENCY_MEASURER_TEMPLATE
OPERAND_RECENCY_MEASURER::operand_recency_measurer(
    IFindFeaturePosition& find_feature_position, IMeasureFeatureRecency& measure_feature_recency)
    : find_feature_position_(find_feature_position)
    , measure_feature_recency_(measure_feature_recency) {}

OPERAND_RECENCY_MEASURER_TEMPLATE
size_t OPERAND_RECENCY_MEASURER::measure(size_t operand) const {
    const std::optional<size_t> position = find_feature_position_.find_feature_position(operand);
    if (!position.has_value())
        return 0;
    return measure_feature_recency_.measure(position.value());
}

#undef OPERAND_RECENCY_MEASURER
#undef OPERAND_RECENCY_MEASURER_TEMPLATE

}

#endif
