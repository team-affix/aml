#ifndef LC_TRANSPILE_BUNDLE_HPP
#define LC_TRANSPILE_BUNDLE_HPP

#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/transpiler.hpp"

struct lc_transpile_bundle {
    lc_expr_pool lc;
    transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
};

#endif
