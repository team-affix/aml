#ifndef DECLARATION_TRANSPILER_HPP
#define DECLARATION_TRANSPILER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/declaration_group.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp>
struct declaration_transpiler {
    declaration_transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app);

    std::vector<const lc_expr*>
    transpile_group(const declaration_group& group);

private:
    IMakeLcVar& make_var_;
    IMakeLcAbs& make_abs_;
    IMakeLcApp& make_app_;

    const lc_expr* transpile_decl(uint32_t group_size, uint32_t position, uint32_t arity);
};

template<typename IV, typename IL, typename IA>
declaration_transpiler<IV, IL, IA>::declaration_transpiler(IV& make_var, IL& make_abs, IA& make_app)
    : make_var_(make_var), make_abs_(make_abs), make_app_(make_app) {}

template<typename IV, typename IL, typename IA>
std::vector<const lc_expr*>
declaration_transpiler<IV, IL, IA>::transpile_group(const declaration_group& group) {
    const auto n = static_cast<uint32_t>(group.declarations.size());
    std::vector<const lc_expr*> result;
    result.reserve(n);
    for (uint32_t k = 0; k < n; ++k)
        result.push_back(transpile_decl(n, k, group.declarations.at(k).arity));
    return result;
}

template<typename IV, typename IL, typename IA>
const lc_expr* declaration_transpiler<IV, IL, IA>::transpile_decl(uint32_t n, uint32_t k,
                                                                   uint32_t a) {
    // Eliminator index for this constructor: a + (n - 1 - k)
    // Applied curried to arity args: var(a-1) ... var(0)
    const lc_expr* body = make_var_.make_var(a + (n - 1 - k));
    for (uint32_t i = a; i > 0; --i)
        body = make_app_.make_app(body, make_var_.make_var(i - 1));
    // Wrap with n eliminator abstractions (outermost) + a arity abstractions (innermost)
    for (uint32_t i = 0; i < n + a; ++i)
        body = make_abs_.make_abs(body);
    return body;
}

#endif
