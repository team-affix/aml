#ifndef FUNCTION_DEF_HPP
#define FUNCTION_DEF_HPP

#include <string>
#include "aml_expr.hpp"

struct function_def {
    std::string     name;
    const aml_expr* body;
};

#endif
