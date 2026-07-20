#ifndef LC_APP_TRANSPILER_HPP
#define LC_APP_TRANSPILER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

template<typename ITranspileLcExpr, typename IMakeFunctor>
struct lc_app_transpiler {
    lc_app_transpiler(ITranspileLcExpr& transpile_lc_expr, IMakeFunctor& make_functor);

    const expr* transpile_app(const lc_expr::app& a);

private:
    ITranspileLcExpr& transpile_lc_expr_;
    IMakeFunctor&     make_functor_;
};

template<typename IT, typename IF>
lc_app_transpiler<IT, IF>::lc_app_transpiler(IT& transpile_lc_expr, IF& make_functor)
    : transpile_lc_expr_(transpile_lc_expr)
    , make_functor_(make_functor) {}

template<typename IT, typename IF>
const expr* lc_app_transpiler<IT, IF>::transpile_app(const lc_expr::app& a) {
    return make_functor_.make_functor(
        k_app_functor_id,
        {transpile_lc_expr_.transpile(a.func), transpile_lc_expr_.transpile(a.arg)});
}

#endif
