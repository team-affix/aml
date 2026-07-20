#ifndef LC_EXPR_TRANSPILER_HPP
#define LC_EXPR_TRANSPILER_HPP

#include <stdexcept>
#include <variant>
#include "infrastructure/lc_abs_transpiler.hpp"
#include "infrastructure/lc_app_transpiler.hpp"
#include "infrastructure/lc_nullptr_transpiler.hpp"
#include "infrastructure/lc_var_transpiler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeVar, typename IMakeFunctor>
struct lc_expr_transpiler {
    using self = lc_expr_transpiler<IMakeVar, IMakeFunctor>;

    using lc_nullptr_transpiler_t = lc_nullptr_transpiler<IMakeVar>;
    using lc_var_transpiler_t     = lc_var_transpiler<IMakeFunctor>;
    using lc_abs_transpiler_t     = lc_abs_transpiler<self, IMakeFunctor>;
    using lc_app_transpiler_t     = lc_app_transpiler<self, IMakeFunctor>;

    lc_expr_transpiler(lc_var_transpiler_t& transpile_lc_var,
                       lc_abs_transpiler_t& transpile_lc_abs,
                       lc_app_transpiler_t& transpile_lc_app,
                       lc_nullptr_transpiler_t& transpile_lc_nullptr);

    const expr* transpile(const lc_expr* e);

private:
    lc_var_transpiler_t&     transpile_lc_var_;
    lc_abs_transpiler_t&     transpile_lc_abs_;
    lc_app_transpiler_t&     transpile_lc_app_;
    lc_nullptr_transpiler_t& transpile_lc_nullptr_;
};

template<typename IV, typename IF>
lc_expr_transpiler<IV, IF>::lc_expr_transpiler(
        lc_var_transpiler_t& transpile_lc_var,
        lc_abs_transpiler_t& transpile_lc_abs,
        lc_app_transpiler_t& transpile_lc_app,
        lc_nullptr_transpiler_t& transpile_lc_nullptr)
    : transpile_lc_var_(transpile_lc_var)
    , transpile_lc_abs_(transpile_lc_abs)
    , transpile_lc_app_(transpile_lc_app)
    , transpile_lc_nullptr_(transpile_lc_nullptr) {}

template<typename IV, typename IF>
const expr* lc_expr_transpiler<IV, IF>::transpile(const lc_expr* e) {
    if (!e)
        return transpile_lc_nullptr_.transpile_nullptr();
    if (const auto* v = std::get_if<lc_expr::var>(&e->content))
        return transpile_lc_var_.transpile_var(*v);
    if (const auto* a = std::get_if<lc_expr::abs>(&e->content))
        return transpile_lc_abs_.transpile_abs(*a);
    if (const auto* ap = std::get_if<lc_expr::app>(&e->content))
        return transpile_lc_app_.transpile_app(*ap);
    throw std::runtime_error("unsupported lc_expr variant");
}

#endif
