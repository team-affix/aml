#ifndef INTEGER_TRANSPILER_HPP
#define INTEGER_TRANSPILER_HPP

#include <cstdint>
#include "infrastructure/global_env.hpp"
#include "infrastructure/lc_global_var.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcVar, typename IMakeLcApp, typename ITranspileNat>
struct integer_transpiler {
    integer_transpiler(IMakeLcVar& make_var, IMakeLcApp& make_app, ITranspileNat& transpile_nat);

    const lc_expr* transpile_integer(const aml_expr::integer& i, const local_binding_env& local,
                                     const global_env& global);

private:
    IMakeLcVar&    make_var_;
    IMakeLcApp&    make_app_;
    ITranspileNat& transpile_nat_;
};

template<typename IV, typename IA, typename IN>
integer_transpiler<IV, IA, IN>::integer_transpiler(IV& make_var, IA& make_app, IN& transpile_nat)
    : make_var_(make_var), make_app_(make_app), transpile_nat_(transpile_nat) {}

template<typename IV, typename IA, typename IN>
const lc_expr* integer_transpiler<IV, IA, IN>::transpile_integer(const aml_expr::integer& i,
                                                                 const local_binding_env& local,
                                                                 const global_env& global) {
    const uint32_t depth = local.depth();
    const int64_t  value = i.value;
    if (value > 0)
        return make_app_.make_app(
            lc_global_var(make_var_, global, "pos", depth),
            transpile_nat_.transpile_nat({static_cast<uint64_t>(value), nat_format::scott}, local,
                                         global));
    if (value < 0)
        return make_app_.make_app(
            lc_global_var(make_var_, global, "negsuc", depth),
            transpile_nat_.transpile_nat({static_cast<uint64_t>(-value - 1), nat_format::scott},
                                         local, global));
    return make_app_.make_app(
        lc_global_var(make_var_, global, "pos", depth),
        transpile_nat_.transpile_nat({0, nat_format::scott}, local, global));
}

#endif
