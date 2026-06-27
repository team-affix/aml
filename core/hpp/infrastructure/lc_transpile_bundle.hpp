#ifndef LC_TRANSPILE_BUNDLE_HPP
#define LC_TRANSPILE_BUNDLE_HPP

#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"

struct lc_transpile_bundle {
    lc_expr_pool lc;
    scope        sc;
    transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool, scope, scope, scope> tx{lc, lc, lc, sc, sc, sc};
};

#endif
