#ifndef TOKEN_TRANSPILER_HPP
#define TOKEN_TRANSPILER_HPP

#include <stdexcept>
#include <string>
#include "infrastructure/global_env.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar>
struct token_transpiler {
    explicit token_transpiler(IMakeLcVar& make_var);

    const lc_expr* transpile_token(const aml_expr::token& t, const local_binding_env& local,
                                   const global_env& global);

private:
    IMakeLcVar& make_var_;

    const lc_expr* resolve_name(const std::string& name, const local_binding_env& local,
                                const global_env& global);
};

template<typename IV>
token_transpiler<IV>::token_transpiler(IV& make_var) : make_var_(make_var) {}

template<typename IV>
const lc_expr* token_transpiler<IV>::resolve_name(const std::string& name,
                                                  const local_binding_env& local,
                                                  const global_env& global) {
    try {
        return make_var_.make_var(local.lookup_local(name));
    } catch (const std::out_of_range&) {
        if (auto k = global.lookup_global(name))
            return make_var_.make_var(local.depth() + *k);
        throw std::runtime_error("unbound name: " + name);
    }
}

template<typename IV>
const lc_expr* token_transpiler<IV>::transpile_token(const aml_expr::token& t,
                                                     const local_binding_env& local,
                                                     const global_env& global) {
    return resolve_name(t.name, local, global);
}

#endif
