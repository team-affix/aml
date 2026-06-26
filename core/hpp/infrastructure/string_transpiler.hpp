#ifndef STRING_TRANSPILER_HPP
#define STRING_TRANSPILER_HPP

#include <cstdint>
#include <string>
#include "infrastructure/global_env.hpp"
#include "infrastructure/lc_global_var.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename ITranspileNat, typename IMakeLcVar, typename IMakeLcApp>
struct string_transpiler {
    string_transpiler(ITranspileNat& transpile_nat, IMakeLcVar& make_var, IMakeLcApp& make_app);

    const lc_expr* transpile_string(const aml_expr::string& s, const local_binding_env& local,
                                    const global_env& global);

private:
    ITranspileNat& transpile_nat_;
    IMakeLcVar&    make_var_;
    IMakeLcApp&    make_app_;
};

template<typename IN, typename IV, typename IA>
string_transpiler<IN, IV, IA>::string_transpiler(IN& transpile_nat, IV& make_var, IA& make_app)
    : transpile_nat_(transpile_nat), make_var_(make_var), make_app_(make_app) {}

template<typename IN, typename IV, typename IA>
const lc_expr* string_transpiler<IN, IV, IA>::transpile_string(const aml_expr::string& s,
                                                                const local_binding_env& local,
                                                                const global_env& global) {
    const lc_expr* list = lc_global_var(make_var_, global, "nil", 0);
    for (auto it = s.value.rbegin(); it != s.value.rend(); ++it) {
        const lc_expr* elem = transpile_nat_.transpile_nat(
            {static_cast<uint64_t>(static_cast<unsigned char>(*it)), nat_format::scott}, local,
            global);
        list = make_app_.make_app(
            make_app_.make_app(lc_global_var(make_var_, global, "cons", 0), elem), list);
    }
    return list;
}

#endif
