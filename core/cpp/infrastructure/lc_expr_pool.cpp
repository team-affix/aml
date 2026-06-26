#include "infrastructure/lc_expr_pool.hpp"

const lc_expr* lc_expr_pool::make_var(uint32_t index) {
    return intern(lc_expr{lc_expr::var{index}});
}

const lc_expr* lc_expr_pool::make_abs(const lc_expr* body) {
    return intern(lc_expr{lc_expr::abs{body}});
}

const lc_expr* lc_expr_pool::make_app(const lc_expr* func, const lc_expr* arg) {
    return intern(lc_expr{lc_expr::app{func, arg}});
}

const lc_expr* lc_expr_pool::intern(lc_expr&& e) {
    return &*exprs.emplace(std::move(e)).first;
}

size_t lc_expr_pool::size() const {
    return exprs.size();
}
