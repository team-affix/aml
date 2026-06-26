#ifndef TRANSPILLED_FUNCTION_HPP
#define TRANSPILLED_FUNCTION_HPP

#include <string>
#include "lc_expr.hpp"

struct transpiled_function {
    std::string    name;
    const lc_expr* body;
};

#endif
