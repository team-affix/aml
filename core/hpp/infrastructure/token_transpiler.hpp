#ifndef TOKEN_TRANSPILER_HPP
#define TOKEN_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IGetVarIndex>
struct token_transpiler {
    token_transpiler(IMakeLcVar& make_var, IGetVarIndex& get_var_index);

    const lc_expr* transpile_token(const aml_expr::token& t);

private:
    IMakeLcVar&   make_var_;
    IGetVarIndex& get_var_index_;
};

template<typename IV, typename IG>
token_transpiler<IV, IG>::token_transpiler(IV& make_var, IG& get_var_index)
    : make_var_(make_var), get_var_index_(get_var_index) {}

template<typename IV, typename IG>
const lc_expr* token_transpiler<IV, IG>::transpile_token(const aml_expr::token& t) {
    return make_var_.make_var(get_var_index_.get_var_index(t.name));
}

#endif
