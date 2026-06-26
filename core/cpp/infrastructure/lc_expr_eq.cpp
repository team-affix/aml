#include "infrastructure/lc_expr_eq.hpp"
#include <type_traits>
#include <variant>

bool lc_expr_eq(const lc_expr* a, const lc_expr* b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;

    return std::visit([&](const auto& av) -> bool {
        using T = std::decay_t<decltype(av)>;
        const auto* bv = std::get_if<T>(&b->content);
        if (!bv)
            return false;
        if constexpr (std::is_same_v<T, lc_expr::var>)
            return av.index == bv->index;
        else if constexpr (std::is_same_v<T, lc_expr::lam>)
            return lc_expr_eq(av.body, bv->body);
        else if constexpr (std::is_same_v<T, lc_expr::app>)
            return lc_expr_eq(av.func, bv->func) && lc_expr_eq(av.arg, bv->arg);
        else
            return false;
    }, a->content);
}
