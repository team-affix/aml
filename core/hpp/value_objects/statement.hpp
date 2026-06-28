#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "aml_expr.hpp"

struct statement {
    const aml_expr* lhs;
    const aml_expr* rhs;
};

#endif
