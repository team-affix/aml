#ifndef FEATURE_SIZER_HPP
#define FEATURE_SIZER_HPP

#include <cstddef>
#include <variant>
#include "value_objects/feature.hpp"

// Sizes a feature: one per variable reference and per primitive application, and a constant's own size.
template<typename ISizeValue>
struct feature_sizer {
    feature_sizer(ISizeValue& size_value);
    size_t size(const feature* expression) const;
private:
    ISizeValue& size_value_;
};

#define FEATURE_SIZER_TEMPLATE template<typename ISizeValue>
#define FEATURE_SIZER          feature_sizer<ISizeValue>

FEATURE_SIZER_TEMPLATE
FEATURE_SIZER::feature_sizer(ISizeValue& size_value)
    : size_value_(size_value) {}

FEATURE_SIZER_TEMPLATE
size_t FEATURE_SIZER::size(const feature* expression) const {
    if (std::holds_alternative<feature::var>(expression->content))
        return 1;
    if (const feature::constant* constant = std::get_if<feature::constant>(&expression->content))
        return size_value_.size(constant->literal);
    size_t total = 1;
    for (const feature* arg : std::get<feature::app>(expression->content).args)
        total += size(arg);
    return total;
}

#undef FEATURE_SIZER
#undef FEATURE_SIZER_TEMPLATE

#endif
