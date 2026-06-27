#ifndef CHURCH_NAT_TRANSPILER_HPP
#define CHURCH_NAT_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp>
struct church_nat_transpiler {
    church_nat_transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app);

    const lc_expr* transpile_nat(const aml_expr::nat& n);

private:
    IMakeLcVar& make_var_;
    IMakeLcAbs& make_abs_;
    IMakeLcApp& make_app_;
};

template<typename IV, typename IL, typename IA>
church_nat_transpiler<IV, IL, IA>::church_nat_transpiler(IV& make_var, IL& make_abs, IA& make_app)
    : make_var_(make_var), make_abs_(make_abs), make_app_(make_app) {}

template<typename IV, typename IL, typename IA>
const lc_expr* church_nat_transpiler<IV, IL, IA>::transpile_nat(const aml_expr::nat& n) {
    const lc_expr* body = make_var_.make_var(0);
    for (uint64_t i = 0; i < n.value; ++i)
        body = make_app_.make_app(make_var_.make_var(1), body);
    return make_abs_.make_abs(make_abs_.make_abs(body));
}

#endif
