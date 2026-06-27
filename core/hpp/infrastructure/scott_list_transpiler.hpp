#ifndef SCOTT_LIST_TRANSPILER_HPP
#define SCOTT_LIST_TRANSPILER_HPP

#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcVar, typename IMakeLcApp, typename IGetVarIndex>
struct scott_list_transpiler {
    scott_list_transpiler(ITranspileExpr& transpile_expr, IMakeLcVar& make_var,
                          IMakeLcApp& make_app, IGetVarIndex& get_var_index);

    const lc_expr* transpile_list(const aml_expr::list& l);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcVar&     make_var_;
    IMakeLcApp&     make_app_;
    IGetVarIndex&   get_var_index_;
};

template<typename IT, typename IV, typename IA, typename IG>
scott_list_transpiler<IT, IV, IA, IG>::scott_list_transpiler(IT& transpile_expr, IV& make_var,
                                                              IA& make_app, IG& get_var_index)
    : transpile_expr_(transpile_expr), make_var_(make_var),
      make_app_(make_app), get_var_index_(get_var_index) {}

template<typename IT, typename IV, typename IA, typename IG>
const lc_expr* scott_list_transpiler<IT, IV, IA, IG>::transpile_list(const aml_expr::list& l) {
    const lc_expr* list = make_var_.make_var(get_var_index_.get_var_index("nil"));
    for (auto it = l.elems.rbegin(); it != l.elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it);
        list = make_app_.make_app(
            make_app_.make_app(make_var_.make_var(get_var_index_.get_var_index("cons")), elem),
            list);
    }
    return list;
}

#endif
