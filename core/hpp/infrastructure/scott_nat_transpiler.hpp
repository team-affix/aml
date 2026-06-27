#ifndef SCOTT_NAT_TRANSPILER_HPP
#define SCOTT_NAT_TRANSPILER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcApp, typename IGetVarIndex>
struct scott_nat_transpiler {
    scott_nat_transpiler(IMakeLcVar& make_var, IMakeLcApp& make_app,
                         IGetVarIndex& get_var_index);

    const lc_expr* transpile_nat(const aml_expr::nat& n);

private:
    IMakeLcVar&   make_var_;
    IMakeLcApp&   make_app_;
    IGetVarIndex& get_var_index_;
};

template<typename IV, typename IA, typename IG>
scott_nat_transpiler<IV, IA, IG>::scott_nat_transpiler(IV& make_var, IA& make_app,
                                                       IG& get_var_index)
    : make_var_(make_var), make_app_(make_app), get_var_index_(get_var_index) {}

template<typename IV, typename IA, typename IG>
const lc_expr* scott_nat_transpiler<IV, IA, IG>::transpile_nat(const aml_expr::nat& n) {
    const lc_expr* list = make_var_.make_var(get_var_index_.get_var_index("nil"));
    if (n.value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = n.value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it
            ? make_var_.make_var(get_var_index_.get_var_index("true"))
            : make_var_.make_var(get_var_index_.get_var_index("false"));
        list = make_app_.make_app(
            make_app_.make_app(make_var_.make_var(get_var_index_.get_var_index("cons")), bit), list);
    }
    return list;
}

#endif
