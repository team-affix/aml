#ifndef LIST_TRANSPILER_HPP
#define LIST_TRANSPILER_HPP

#include <stdexcept>
#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/list_decl_group.hpp"

template<typename ITranspileExpr, typename IMakeLcVar, typename IMakeLcAbs,
         typename IMakeLcApp, typename IGetVarIndex>
struct list_transpiler {
    list_transpiler(ITranspileExpr& transpile_expr, IMakeLcVar& make_var,
                    IMakeLcAbs& make_abs, IMakeLcApp& make_app,
                    IGetVarIndex& get_var_index);

    const lc_expr* transpile_list(const aml_expr::list& l);

private:
    const lc_expr* transpile_scott(const std::vector<const aml_expr*>& elems);
    const lc_expr* transpile_church(const std::vector<const aml_expr*>& elems);

    ITranspileExpr& transpile_expr_;
    IMakeLcVar&     make_var_;
    IMakeLcAbs&     make_abs_;
    IMakeLcApp&     make_app_;
    IGetVarIndex&   get_var_index_;
};

template<typename IT, typename IV, typename IL, typename IA, typename IG>
list_transpiler<IT, IV, IL, IA, IG>::list_transpiler(IT& transpile_expr, IV& make_var,
                                                     IL& make_abs, IA& make_app,
                                                     IG& get_var_index)
    : transpile_expr_(transpile_expr)
    , make_var_(make_var)
    , make_abs_(make_abs)
    , make_app_(make_app)
    , get_var_index_(get_var_index) {}

template<typename IT, typename IV, typename IL, typename IA, typename IG>
const lc_expr* list_transpiler<IT, IV, IL, IA, IG>::transpile_list(
        const aml_expr::list& l) {
    switch (l.format) {
        case list_format::scott:  return transpile_scott(l.elems);
        case list_format::church: return transpile_church(l.elems);
    }
    throw std::runtime_error("unsupported list_format");
}

template<typename IT, typename IV, typename IL, typename IA, typename IG>
const lc_expr* list_transpiler<IT, IV, IL, IA, IG>::transpile_scott(
        const std::vector<const aml_expr*>& elems) {
    const lc_expr* list = make_var_.make_var(get_var_index_.get_var_index(k_nil_name));
    for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it);
        list = make_app_.make_app(
            make_app_.make_app(make_var_.make_var(get_var_index_.get_var_index(k_cons_name)), elem),
            list);
    }
    return list;
}

template<typename IT, typename IV, typename IL, typename IA, typename IG>
const lc_expr* list_transpiler<IT, IV, IL, IA, IG>::transpile_church(
        const std::vector<const aml_expr*>& elems) {
    const lc_expr* body = make_var_.make_var(0);
    for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
        const lc_expr* elem = transpile_expr_.transpile(*it);
        body = make_app_.make_app(make_app_.make_app(make_var_.make_var(1), elem), body);
    }
    return make_abs_.make_abs(make_abs_.make_abs(body));
}

#endif
