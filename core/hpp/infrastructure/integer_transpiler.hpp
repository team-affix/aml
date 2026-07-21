#ifndef INTEGER_TRANSPILER_HPP
#define INTEGER_TRANSPILER_HPP

#include <cstdint>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcApp, typename ITranspileNat,
         typename ITranspilePos, typename ITranspileNegsuc>
struct integer_transpiler {
    integer_transpiler(IMakeLcApp& make_app, ITranspileNat& transpile_nat,
                       ITranspilePos& transpile_pos, ITranspileNegsuc& transpile_negsuc);

    const lc_expr* transpile_integer(const aml_expr::integer& i);

private:
    IMakeLcApp&         make_app_;
    ITranspileNat&      transpile_nat_;
    ITranspilePos&      transpile_pos_;
    ITranspileNegsuc&   transpile_negsuc_;
};

template<typename IA, typename IN, typename IP, typename INeg>
integer_transpiler<IA, IN, IP, INeg>::integer_transpiler(
        IA& make_app, IN& transpile_nat, IP& transpile_pos, INeg& transpile_negsuc)
    : make_app_(make_app)
    , transpile_nat_(transpile_nat)
    , transpile_pos_(transpile_pos)
    , transpile_negsuc_(transpile_negsuc) {}

template<typename IA, typename IN, typename IP, typename INeg>
const lc_expr* integer_transpiler<IA, IN, IP, INeg>::transpile_integer(
        const aml_expr::integer& i) {
    const int64_t value = i.value;
    if (value >= 0)
        return make_app_.make_app(
            transpile_pos_.transpile_pos(),
            transpile_nat_.transpile_nat({static_cast<uint64_t>(value), i.format}));
    return make_app_.make_app(
        transpile_negsuc_.transpile_negsuc(),
        transpile_nat_.transpile_nat({static_cast<uint64_t>(-value - 1), i.format}));
}

#endif
