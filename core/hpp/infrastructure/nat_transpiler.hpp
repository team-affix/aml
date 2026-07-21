#ifndef NAT_TRANSPILER_HPP
#define NAT_TRANSPILER_HPP

#include <cstdint>
#include <stdexcept>
#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/bool_decl_group.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp, typename IGetVarIndex>
struct nat_transpiler {
    nat_transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app,
                   IGetVarIndex& get_var_index);

    const lc_expr* transpile_nat(const aml_expr::nat& n);

private:
    const lc_expr* transpile_binary(uint64_t value);
    const lc_expr* transpile_church(uint64_t value);

    IMakeLcVar&   make_var_;
    IMakeLcAbs&   make_abs_;
    IMakeLcApp&   make_app_;
    IGetVarIndex& get_var_index_;
};

template<typename IV, typename IL, typename IA, typename IG>
nat_transpiler<IV, IL, IA, IG>::nat_transpiler(IV& make_var, IL& make_abs, IA& make_app,
                                               IG& get_var_index)
    : make_var_(make_var)
    , make_abs_(make_abs)
    , make_app_(make_app)
    , get_var_index_(get_var_index) {}

template<typename IV, typename IL, typename IA, typename IG>
const lc_expr* nat_transpiler<IV, IL, IA, IG>::transpile_nat(const aml_expr::nat& n) {
    switch (n.format) {
        case nat_format::binary: return transpile_binary(n.value);
        case nat_format::church: return transpile_church(n.value);
    }
    throw std::runtime_error("unsupported nat_format");
}

template<typename IV, typename IL, typename IA, typename IG>
const lc_expr* nat_transpiler<IV, IL, IA, IG>::transpile_binary(uint64_t value) {
    const lc_expr* list = make_var_.make_var(get_var_index_.get_var_index(k_nil_name));
    if (value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it
            ? make_var_.make_var(get_var_index_.get_var_index(k_true_name))
            : make_var_.make_var(get_var_index_.get_var_index(k_false_name));
        list = make_app_.make_app(
            make_app_.make_app(make_var_.make_var(get_var_index_.get_var_index(k_cons_name)), bit),
            list);
    }
    return list;
}

template<typename IV, typename IL, typename IA, typename IG>
const lc_expr* nat_transpiler<IV, IL, IA, IG>::transpile_church(uint64_t value) {
    const lc_expr* body = make_var_.make_var(0);
    for (uint64_t i = 0; i < value; ++i)
        body = make_app_.make_app(make_var_.make_var(1), body);
    return make_abs_.make_abs(make_abs_.make_abs(body));
}

#endif
