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
template<typename IFindUniformValue>
struct leaf_detector {
    leaf_detector(IFindUniformValue& find_uniform_value);
    std::optional<feature> detect(const bin& data) const;
private:
    IFindUniformValue& find_uniform_value_;
};

#define LEAF_DETECTOR_TEMPLATE template<typename IFindUniformValue>
#define LEAF_DETECTOR          leaf_detector<IFindUniformValue>

LEAF_DETECTOR_TEMPLATE
LEAF_DETECTOR::leaf_detector(IFindUniformValue& find_uniform_value)
    : find_uniform_value_(find_uniform_value) {}

LEAF_DETECTOR_TEMPLATE
std::optional<feature> LEAF_DETECTOR::detect(const bin& data) const {
    for (var_id index = 0; index < data.inputs.size(); ++index) {
        if (data.inputs[index] != data.labels)
            continue;
        return feature{feature::var{.index = index}};
    }
    const std::optional<const value*> label = find_uniform_value_.find(data.labels.cells);
    if (!label.has_value())
        return std::nullopt;
    return feature{feature::constant{.literal = label.value()}};
}

#undef LEAF_DETECTOR
#undef LEAF_DETECTOR_TEMPLATE

#endif
