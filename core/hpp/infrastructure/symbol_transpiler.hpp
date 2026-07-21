#ifndef SYMBOL_TRANSPILER_HPP
#define SYMBOL_TRANSPILER_HPP

#include <stdexcept>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IContainsName, typename IGetVarIndex,
         typename ITryTranspileBuiltin>
struct symbol_transpiler {
    symbol_transpiler(IMakeLcVar& make_var, IContainsName& contains_name,
                      IGetVarIndex& get_var_index,
                      ITryTranspileBuiltin& try_transpile_builtin);

    const lc_expr* transpile_symbol(const aml_expr::symbol& t);

private:
    IMakeLcVar&            make_var_;
    IContainsName&         contains_name_;
    IGetVarIndex&          get_var_index_;
    ITryTranspileBuiltin&  try_transpile_builtin_;
};

template<typename IV, typename IC, typename IG, typename IB>
symbol_transpiler<IV, IC, IG, IB>::symbol_transpiler(
        IV& make_var, IC& contains_name, IG& get_var_index, IB& try_transpile_builtin)
    : make_var_(make_var)
    , contains_name_(contains_name)
    , get_var_index_(get_var_index)
    , try_transpile_builtin_(try_transpile_builtin) {}

template<typename IV, typename IC, typename IG, typename IB>
const lc_expr* symbol_transpiler<IV, IC, IG, IB>::transpile_symbol(
        const aml_expr::symbol& t) {
    if (contains_name_.contains(t.name))
        return make_var_.make_var(get_var_index_.get_var_index(t.name));
    if (const lc_expr* builtin = try_transpile_builtin_.try_transpile_name(t.name))
        return builtin;
    throw std::out_of_range("unbound symbol: " + t.name);
}

#endif
