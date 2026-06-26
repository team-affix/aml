#ifndef SCOTT_LIST_TRANSPILER_HPP
#define SCOTT_LIST_TRANSPILER_HPP

#include <vector>
#include "infrastructure/global_env.hpp"
#include "infrastructure/lc_global_var.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcVar, typename IMakeLcApp>
struct scott_list_transpiler {
    scott_list_transpiler(ITranspileExpr& transpile_expr, IMakeLcVar& make_var,
                          IMakeLcApp& make_app);

    const lc_expr* transpile_list(const aml_expr::list& l, const local_binding_env& local,
                                  const global_env& global);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcVar&     make_var_;
    IMakeLcApp&     make_app_;
};

template<typename IT, typename IV, typename IA>
scott_list_transpiler<IT, IV, IA>::scott_list_transpiler(IT& transpile_expr, IV& make_var,
                                                         IA& make_app)
    : transpile_expr_(transpile_expr), make_var_(make_var), make_app_(make_app) {}

template<typename IT, typename IV, typename IA>
const lc_expr* scott_list_transpiler<IT, IV, IA>::transpile_list(const aml_expr::list& l,
                                                                 const local_binding_env& local,
                                                                 const global_env& global) {
    const lc_expr* list = lc_global_var(make_var_, global, "nil", 0);
    for (auto it = l.elems.rbegin(); it != l.elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it, local, global);
        list = make_app_.make_app(
            make_app_.make_app(lc_global_var(make_var_, global, "cons", 0), elem), list);
    }
    return list;
}

#endif
