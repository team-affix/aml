#ifndef SYMBOL_TRANSPILER_HPP
#define SYMBOL_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IGetVarIndex>
struct symbol_transpiler {
    symbol_transpiler(IMakeLcVar& make_var, IGetVarIndex& get_var_index);

    const lc_expr* transpile_symbol(const aml_expr::symbol& t);

private:
    IMakeLcVar&   make_var_;
    IGetVarIndex& get_var_index_;
};

template<typename IV, typename IG>
symbol_transpiler<IV, IG>::symbol_transpiler(IV& make_var, IG& get_var_index)
    : make_var_(make_var), get_var_index_(get_var_index) {}

template<typename IV, typename IG>
const lc_expr* symbol_transpiler<IV, IG>::transpile_symbol(const aml_expr::symbol& t) {
    return make_var_.make_var(get_var_index_.get_var_index(t.name));
}

#endif
