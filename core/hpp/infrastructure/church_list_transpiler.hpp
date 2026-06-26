#ifndef CHURCH_LIST_TRANSPILER_HPP
#define CHURCH_LIST_TRANSPILER_HPP

#include "infrastructure/global_env.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcVar, typename IMakeLcLam, typename IMakeLcApp>
struct church_list_transpiler {
    church_list_transpiler(ITranspileExpr& transpile_expr, IMakeLcVar& make_var,
                           IMakeLcLam& make_lam, IMakeLcApp& make_app);

    const lc_expr* transpile_list(const aml_expr::list& l, const local_binding_env& local,
                                  const global_env& global);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcVar&     make_var_;
    IMakeLcLam&     make_lam_;
    IMakeLcApp&     make_app_;
};

template<typename IT, typename IV, typename IL, typename IA>
church_list_transpiler<IT, IV, IL, IA>::church_list_transpiler(IT& transpile_expr, IV& make_var,
                                                               IL& make_lam, IA& make_app)
    : transpile_expr_(transpile_expr),
      make_var_(make_var),
      make_lam_(make_lam),
      make_app_(make_app) {}

template<typename IT, typename IV, typename IL, typename IA>
const lc_expr* church_list_transpiler<IT, IV, IL, IA>::transpile_list(const aml_expr::list& l,
                                                                      const local_binding_env& local,
                                                                      const global_env& global) {
    const lc_expr* body = make_var_.make_var(0);
    for (auto it = l.elems.rbegin(); it != l.elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it, local, global);
        body = make_app_.make_app(make_app_.make_app(make_var_.make_var(1), elem), body);
    }
    return make_lam_.make_lam(make_lam_.make_lam(body));
}

#endif
