#ifndef FEATURE_QUADRANT_MEASURER_HPP
#define FEATURE_QUADRANT_MEASURER_HPP

#include <cstddef>

namespace AML {

// Measures the quadrant key of the session's feature at a position: that of the feature's operand.
template<typename IGetFeatureOperand, typename IMeasureOperandQuadrant>
struct feature_quadrant_measurer {
    feature_quadrant_measurer(
        IGetFeatureOperand& get_feature_operand, IMeasureOperandQuadrant& measure_operand_quadrant);
    size_t measure(size_t position) const;
private:
    IGetFeatureOperand& get_feature_operand_;
    IMeasureOperandQuadrant& measure_operand_quadrant_;
};

#define FEATURE_QUADRANT_MEASURER_TEMPLATE template<typename IGetFeatureOperand, typename IMeasureOperandQuadrant>
#define FEATURE_QUADRANT_MEASURER          feature_quadrant_measurer<IGetFeatureOperand, IMeasureOperandQuadrant>

FEATURE_QUADRANT_MEASURER_TEMPLATE
FEATURE_QUADRANT_MEASURER::feature_quadrant_measurer(
    IGetFeatureOperand& get_feature_operand, IMeasureOperandQuadrant& measure_operand_quadrant)
    : get_feature_operand_(get_feature_operand)
    , measure_operand_quadrant_(measure_operand_quadrant) {}

FEATURE_QUADRANT_MEASURER_TEMPLATE
size_t FEATURE_QUADRANT_MEASURER::measure(size_t position) const {
    return measure_operand_quadrant_.measure(get_feature_operand_.feature_operand(position));
}

#undef FEATURE_QUADRANT_MEASURER
#undef FEATURE_QUADRANT_MEASURER_TEMPLATE

}

#endif
