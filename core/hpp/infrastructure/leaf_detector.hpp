#ifndef LEAF_DETECTOR_HPP
#define LEAF_DETECTOR_HPP

#include <optional>
#include "value_objects/bin.hpp"
#include "value_objects/value.hpp"

// Detects a bin whose labels all agree, returning that label.
template<typename IFindUniformValue>
struct leaf_detector {
    leaf_detector(IFindUniformValue& find_uniform_value);
    std::optional<const value*> detect(const bin& data) const;
private:
    IFindUniformValue& find_uniform_value_;
};

#define LEAF_DETECTOR_TEMPLATE template<typename IFindUniformValue>
#define LEAF_DETECTOR          leaf_detector<IFindUniformValue>

LEAF_DETECTOR_TEMPLATE
LEAF_DETECTOR::leaf_detector(IFindUniformValue& find_uniform_value)
    : find_uniform_value_(find_uniform_value) {}

LEAF_DETECTOR_TEMPLATE
std::optional<const value*> LEAF_DETECTOR::detect(const bin& data) const {
    return find_uniform_value_.find(data.labels);
}

#undef LEAF_DETECTOR
#undef LEAF_DETECTOR_TEMPLATE

#endif
