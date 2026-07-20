#ifndef DATA_POINT_TRANSPILER_HPP
#define DATA_POINT_TRANSPILER_HPP

#include <vector>
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/data_point.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

template<typename ITranspileLcExpr, typename IMakeVar, typename IMakeFunctor>
struct data_point_transpiler {
    data_point_transpiler(ITranspileLcExpr& transpile_lc_expr,
                          IMakeVar& make_var,
                          IMakeFunctor& make_functor);

    const expr* transpile_data_point(const data_point& dp);

private:
    ITranspileLcExpr& transpile_lc_expr_;
    IMakeVar&         make_var_;
    IMakeFunctor&     make_functor_;
};

template<typename IT, typename IV, typename IF>
data_point_transpiler<IT, IV, IF>::data_point_transpiler(
        IT& transpile_lc_expr, IV& make_var, IF& make_functor)
    : transpile_lc_expr_(transpile_lc_expr)
    , make_var_(make_var)
    , make_functor_(make_functor) {}

template<typename IT, typename IV, typename IF>
const expr* data_point_transpiler<IT, IV, IF>::transpile_data_point(
        const data_point& dp) {
    const expr* x = transpile_lc_expr_.transpile(dp.input);
    const expr* y = transpile_lc_expr_.transpile(dp.label);
    const expr* M = make_var_.make_var(k_model_var_id);
    const expr* applied =
        make_functor_.make_functor(k_app_functor_id, {M, x});
    return make_functor_.make_functor(k_normalize_functor_id, {applied, y});
}

#endif
