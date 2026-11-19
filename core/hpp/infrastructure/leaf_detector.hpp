#ifndef LEAF_DETECTOR_HPP
#define LEAF_DETECTOR_HPP

#include <optional>
#include "value_objects/bin.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/value.hpp"
#include "value_objects/var_id.hpp"

// Detects a bin that needs no partition and gives its output. A variable whose column is the label column is the
// output (the first such, at cost one, which no constant undercuts); otherwise labels that all agree give that label
// as a constant. A variable output generalizes past the training data; a constant one does not.
template<typename IMakeVarFeature, typename IFindUniformValue, typename IMakeConstantFeature>
struct leaf_detector {
    leaf_detector(
        IMakeVarFeature& make_var_feature,
        IFindUniformValue& find_uniform_value,
        IMakeConstantFeature& make_constant_feature);
    std::optional<const feature*> detect(const bin& data) const;
private:
    IMakeVarFeature& make_var_feature_;
    IFindUniformValue& find_uniform_value_;
    IMakeConstantFeature& make_constant_feature_;
};

#define LEAF_DETECTOR_TEMPLATE template<typename IMVF, typename IFUV, typename IMCF>
#define LEAF_DETECTOR          leaf_detector<IMVF, IFUV, IMCF>

LEAF_DETECTOR_TEMPLATE
LEAF_DETECTOR::leaf_detector(IMVF& make_var_feature, IFUV& find_uniform_value, IMCF& make_constant_feature)
    : make_var_feature_(make_var_feature)
    , find_uniform_value_(find_uniform_value)
    , make_constant_feature_(make_constant_feature) {}

LEAF_DETECTOR_TEMPLATE
std::optional<const feature*> LEAF_DETECTOR::detect(const bin& data) const {
    for (var_id index = 0; index < data.inputs.size(); ++index) {
        if (data.inputs[index] != data.labels)
            continue;
        return make_var_feature_.make_var(index);
    }
    const std::optional<const value*> label = find_uniform_value_.find(data.labels.cells);
    if (!label.has_value())
        return std::nullopt;
    return make_constant_feature_.make_constant(label.value());
}

#undef LEAF_DETECTOR
#undef LEAF_DETECTOR_TEMPLATE

#endif
