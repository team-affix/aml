#ifndef INTEGER_TRANSPILER_HPP
#define INTEGER_TRANSPILER_HPP

#include <cstdint>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/int_decl_group.hpp"

template<typename IMakeLcVar, typename IMakeLcApp, typename ITranspileNat, typename IGetVarIndex>
struct integer_transpiler {
    integer_transpiler(IMakeLcVar& make_var, IMakeLcApp& make_app,
                       ITranspileNat& transpile_nat, IGetVarIndex& get_var_index);

    const lc_expr* transpile_integer(const aml_expr::integer& i);

private:
    IMakeLcVar&    make_var_;
    IMakeLcApp&    make_app_;
    ITranspileNat& transpile_nat_;
    IGetVarIndex&  get_var_index_;
};

template<typename IV, typename IA, typename IN, typename IG>
integer_transpiler<IV, IA, IN, IG>::integer_transpiler(IV& make_var, IA& make_app,
                                                       IN& transpile_nat, IG& get_var_index)
    : make_var_(make_var), make_app_(make_app),
      transpile_nat_(transpile_nat), get_var_index_(get_var_index) {}

template<typename IV, typename IA, typename IN, typename IG>
const lc_expr* integer_transpiler<IV, IA, IN, IG>::transpile_integer(const aml_expr::integer& i) {
    const int64_t value = i.value;
    if (value >= 0)
        return make_app_.make_app(
            make_var_.make_var(get_var_index_.get_var_index(k_pos_name)),
            transpile_nat_.transpile_nat({static_cast<uint64_t>(value), i.format}));
    return make_app_.make_app(
        make_var_.make_var(get_var_index_.get_var_index(k_negsuc_name)),
        transpile_nat_.transpile_nat({static_cast<uint64_t>(-value - 1), i.format}));
}

#endif
