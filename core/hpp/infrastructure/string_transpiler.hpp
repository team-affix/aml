#ifndef STRING_TRANSPILER_HPP
#define STRING_TRANSPILER_HPP

#include <cstdint>
#include <string>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename ITranspileNat, typename IMakeLcApp,
         typename ITranspileNil, typename ITranspileCons>
struct string_transpiler {
    string_transpiler(ITranspileNat& transpile_nat, IMakeLcApp& make_app,
                      ITranspileNil& transpile_nil, ITranspileCons& transpile_cons);

    const lc_expr* transpile_string(const aml_expr::string& s);

private:
    ITranspileNat&  transpile_nat_;
    IMakeLcApp&     make_app_;
    ITranspileNil&  transpile_nil_;
    ITranspileCons& transpile_cons_;
};

template<typename IN, typename IA, typename INil, typename IC>
string_transpiler<IN, IA, INil, IC>::string_transpiler(
        IN& transpile_nat, IA& make_app, INil& transpile_nil, IC& transpile_cons)
    : transpile_nat_(transpile_nat)
    , make_app_(make_app)
    , transpile_nil_(transpile_nil)
    , transpile_cons_(transpile_cons) {}

template<typename IN, typename IA, typename INil, typename IC>
const lc_expr* string_transpiler<IN, IA, INil, IC>::transpile_string(
        const aml_expr::string& s) {
    const lc_expr* list = transpile_nil_.transpile_nil();
    for (auto it = s.value.rbegin(); it != s.value.rend(); ++it) {
        const lc_expr* elem = transpile_nat_.transpile_nat(
            {static_cast<uint64_t>(static_cast<unsigned char>(*it)), nat_format::binary});
        list = make_app_.make_app(
            make_app_.make_app(transpile_cons_.transpile_cons(), elem),
            list);
    }
    return list;
}

#endif
