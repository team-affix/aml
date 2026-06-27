#ifndef CHURCH_LIST_TRANSPILER_HPP
#define CHURCH_LIST_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp>
struct church_list_transpiler {
    church_list_transpiler(ITranspileExpr& transpile_expr, IMakeLcVar& make_var,
                           IMakeLcAbs& make_abs, IMakeLcApp& make_app);

    const lc_expr* transpile_list(const aml_expr::list& l);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcVar&     make_var_;
    IMakeLcAbs&     make_abs_;
    IMakeLcApp&     make_app_;
};

template<typename IT, typename IV, typename IL, typename IA>
church_list_transpiler<IT, IV, IL, IA>::church_list_transpiler(IT& transpile_expr, IV& make_var,
                                                               IL& make_abs, IA& make_app)
    : transpile_expr_(transpile_expr),
      make_var_(make_var),
      make_abs_(make_abs),
      make_app_(make_app) {}

template<typename IT, typename IV, typename IL, typename IA>
const lc_expr* church_list_transpiler<IT, IV, IL, IA>::transpile_list(const aml_expr::list& l) {
    const lc_expr* body = make_var_.make_var(0);
    for (auto it = l.elems.rbegin(); it != l.elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it);
        body = make_app_.make_app(make_app_.make_app(make_var_.make_var(1), elem), body);
    }
    return make_abs_.make_abs(make_abs_.make_abs(body));
}

#endif
