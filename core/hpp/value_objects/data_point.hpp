#ifndef DATA_POINT_HPP
#define DATA_POINT_HPP

#include "value_objects/lc_expr.hpp"

struct data_point {
    const lc_expr* input;
    const lc_expr* label;
    auto operator<=>(const data_point&) const = default;
};

#endif
