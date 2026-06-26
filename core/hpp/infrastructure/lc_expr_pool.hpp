#ifndef LC_EXPR_POOL_HPP
#define LC_EXPR_POOL_HPP

#include <set>
#include <vector>
#include "value_objects/lc_expr.hpp"

struct lc_expr_pool {
    const lc_expr* make_var(uint32_t index);
    const lc_expr* make_lam(const lc_expr* body);
    const lc_expr* make_app(const lc_expr* func, const lc_expr* arg);
    size_t size() const;

    lc_expr_pool() = default;
    lc_expr_pool(const lc_expr_pool&) = delete;
    lc_expr_pool& operator=(const lc_expr_pool&) = delete;
    lc_expr_pool(lc_expr_pool&&) = default;
    lc_expr_pool& operator=(lc_expr_pool&&) = default;

private:
    const lc_expr* intern(lc_expr&& e);
    std::set<lc_expr> exprs;
};

#endif
