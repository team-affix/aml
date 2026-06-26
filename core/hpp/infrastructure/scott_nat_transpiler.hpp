#ifndef SCOTT_NAT_TRANSPILER_HPP
#define SCOTT_NAT_TRANSPILER_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/global_env.hpp"
#include "infrastructure/lc_global_var.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcApp>
struct scott_nat_transpiler {
    scott_nat_transpiler(IMakeLcVar& make_var, IMakeLcApp& make_app);

    const lc_expr* transpile_nat(const aml_expr::nat& n, const local_binding_env& local,
                                 const global_env& global);

private:
    IMakeLcVar& make_var_;
    IMakeLcApp& make_app_;
};

template<typename IV, typename IA>
scott_nat_transpiler<IV, IA>::scott_nat_transpiler(IV& make_var, IA& make_app)
    : make_var_(make_var), make_app_(make_app) {}

template<typename IV, typename IA>
const lc_expr* scott_nat_transpiler<IV, IA>::transpile_nat(const aml_expr::nat& n,
                                                           const local_binding_env&,
                                                           const global_env& global) {
    const lc_expr* list = lc_global_var(make_var_, global, "nil", 0);
    if (n.value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = n.value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it ? lc_global_var(make_var_, global, "true", 0)
                                 : lc_global_var(make_var_, global, "false", 0);
        list = make_app_.make_app(
            make_app_.make_app(lc_global_var(make_var_, global, "cons", 0), bit), list);
    }
    return list;
}

#endif
