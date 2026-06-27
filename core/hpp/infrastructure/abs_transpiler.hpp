#ifndef ABS_TRANSPILER_HPP
#define ABS_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcAbs, typename IPushVar, typename IPopVar>
struct abs_transpiler {
    abs_transpiler(ITranspileExpr& transpile_expr, IMakeLcAbs& make_abs,
                   IPushVar& push_var, IPopVar& pop_var);

    const lc_expr* transpile_abs(const aml_expr::abs& a);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcAbs&     make_abs_;
    IPushVar&       push_var_;
    IPopVar&        pop_var_;
};

template<typename IT, typename IL, typename IP, typename IO>
abs_transpiler<IT, IL, IP, IO>::abs_transpiler(IT& transpile_expr, IL& make_abs,
                                               IP& push_var, IO& pop_var)
    : transpile_expr_(transpile_expr), make_abs_(make_abs),
      push_var_(push_var), pop_var_(pop_var) {}

template<typename IT, typename IL, typename IP, typename IO>
const lc_expr* abs_transpiler<IT, IL, IP, IO>::transpile_abs(const aml_expr::abs& a) {
    push_var_.push(a.param);
    const lc_expr* result = make_abs_.make_abs(transpile_expr_.transpile(a.body));
    pop_var_.pop();
    return result;
}

#endif
