#ifndef ABS_TRANSPILER_HPP
#define ABS_TRANSPILER_HPP

#include "infrastructure/global_env.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcLam>
struct abs_transpiler {
    abs_transpiler(ITranspileExpr& transpile_expr, IMakeLcLam& make_lam);

    const lc_expr* transpile_abs(const aml_expr::abs& a, const local_binding_env& local,
                                 const global_env& global);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcLam&     make_lam_;
};

template<typename IT, typename IL>
abs_transpiler<IT, IL>::abs_transpiler(IT& transpile_expr, IL& make_lam)
    : transpile_expr_(transpile_expr), make_lam_(make_lam) {}

template<typename IT, typename IL>
const lc_expr* abs_transpiler<IT, IL>::transpile_abs(const aml_expr::abs& a,
                                                     const local_binding_env& local,
                                                     const global_env& global) {
    local_binding_env inner = local;
    inner.push(a.param);
    return make_lam_.make_lam(transpile_expr_.transpile(a.body, inner, global));
}

#endif
