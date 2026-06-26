#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "aml_expr.hpp"

struct statement {
    const aml_expr* input;
    const aml_expr* label;
};

#endif
