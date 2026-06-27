#ifndef STRING_TRANSPILER_HPP
#define STRING_TRANSPILER_HPP

#include <cstdint>
#include <string>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename ITranspileNat, typename IMakeLcVar, typename IMakeLcApp, typename IGetVarIndex>
struct string_transpiler {
    string_transpiler(ITranspileNat& transpile_nat, IMakeLcVar& make_var,
                      IMakeLcApp& make_app, IGetVarIndex& get_var_index);

    const lc_expr* transpile_string(const aml_expr::string& s);

private:
    ITranspileNat& transpile_nat_;
    IMakeLcVar&    make_var_;
    IMakeLcApp&    make_app_;
    IGetVarIndex&  get_var_index_;
};

template<typename IN, typename IV, typename IA, typename IG>
string_transpiler<IN, IV, IA, IG>::string_transpiler(IN& transpile_nat, IV& make_var,
                                                      IA& make_app, IG& get_var_index)
    : transpile_nat_(transpile_nat), make_var_(make_var),
      make_app_(make_app), get_var_index_(get_var_index) {}

template<typename IN, typename IV, typename IA, typename IG>
const lc_expr* string_transpiler<IN, IV, IA, IG>::transpile_string(const aml_expr::string& s) {
    const lc_expr* list = make_var_.make_var(get_var_index_.get_var_index("nil"));
    for (auto it = s.value.rbegin(); it != s.value.rend(); ++it) {
        const lc_expr* elem = transpile_nat_.transpile_nat(
            {static_cast<uint64_t>(static_cast<unsigned char>(*it)), nat_format::scott});
        list = make_app_.make_app(
            make_app_.make_app(make_var_.make_var(get_var_index_.get_var_index("cons")), elem),
            list);
    }
    return list;
}

#endif
