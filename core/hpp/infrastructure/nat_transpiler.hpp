#ifndef NAT_TRANSPILER_HPP
#define NAT_TRANSPILER_HPP

#include <cstdint>
#include <stdexcept>
#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp,
         typename ITranspileNil, typename ITranspileCons,
         typename ITranspileTrue, typename ITranspileFalse>
struct nat_transpiler {
    nat_transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app,
                   ITranspileNil& transpile_nil, ITranspileCons& transpile_cons,
                   ITranspileTrue& transpile_true, ITranspileFalse& transpile_false);

    const lc_expr* transpile_nat(const aml_expr::nat& n);

private:
    const lc_expr* transpile_binary(uint64_t value);
    const lc_expr* transpile_church(uint64_t value);

    IMakeLcVar&      make_var_;
    IMakeLcAbs&      make_abs_;
    IMakeLcApp&      make_app_;
    ITranspileNil&   transpile_nil_;
    ITranspileCons&  transpile_cons_;
    ITranspileTrue&  transpile_true_;
    ITranspileFalse& transpile_false_;
};

template<typename IV, typename IL, typename IA, typename IN, typename IC, typename IT, typename IF>
nat_transpiler<IV, IL, IA, IN, IC, IT, IF>::nat_transpiler(
        IV& make_var, IL& make_abs, IA& make_app,
        IN& transpile_nil, IC& transpile_cons,
        IT& transpile_true, IF& transpile_false)
    : make_var_(make_var)
    , make_abs_(make_abs)
    , make_app_(make_app)
    , transpile_nil_(transpile_nil)
    , transpile_cons_(transpile_cons)
    , transpile_true_(transpile_true)
    , transpile_false_(transpile_false) {}

template<typename IV, typename IL, typename IA, typename IN, typename IC, typename IT, typename IF>
const lc_expr* nat_transpiler<IV, IL, IA, IN, IC, IT, IF>::transpile_nat(
        const aml_expr::nat& n) {
    switch (n.format) {
        case nat_format::binary: return transpile_binary(n.value);
        case nat_format::church: return transpile_church(n.value);
    }
    throw std::runtime_error("unsupported nat_format");
}

template<typename IV, typename IL, typename IA, typename IN, typename IC, typename IT, typename IF>
const lc_expr* nat_transpiler<IV, IL, IA, IN, IC, IT, IF>::transpile_binary(uint64_t value) {
    const lc_expr* list = transpile_nil_.transpile_nil();
    if (value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it
            ? transpile_true_.transpile_true()
            : transpile_false_.transpile_false();
        list = make_app_.make_app(
            make_app_.make_app(transpile_cons_.transpile_cons(), bit),
            list);
    }
    return list;
}

template<typename IV, typename IL, typename IA, typename IN, typename IC, typename IT, typename IF>
const lc_expr* nat_transpiler<IV, IL, IA, IN, IC, IT, IF>::transpile_church(uint64_t value) {
    const lc_expr* body = make_var_.make_var(0);
    for (uint64_t i = 0; i < value; ++i)
        body = make_app_.make_app(make_var_.make_var(1), body);
    return make_abs_.make_abs(make_abs_.make_abs(body));
}

#endif
