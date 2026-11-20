#ifndef FEATURE_ORDER_MEASURER_HPP
#define FEATURE_ORDER_MEASURER_HPP

#include <cstddef>
#include "value_objects/feature.hpp"

namespace AML {

// Measures a feature's order: a variable or a constant is order zero, being there before any synthesis, and a
// primitive application is one more than its highest-order argument.
struct feature_order_measurer {
    size_t measure(const feature& expression) const;
};

}

#endif
