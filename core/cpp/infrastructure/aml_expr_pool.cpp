#include "infrastructure/aml_expr_pool.hpp"

const aml_expr* aml_expr_pool::make_app(const aml_expr* func, const aml_expr* arg) {
    return intern(aml_expr{aml_expr::app{func, arg}});
}

const aml_expr* aml_expr_pool::make_abs(std::string param, const aml_expr* body) {
    return intern(aml_expr{aml_expr::abs{std::move(param), body}});
}

const aml_expr* aml_expr_pool::make_var(std::string name) {
    return intern(aml_expr{aml_expr::var{std::move(name)}});
}

const aml_expr* aml_expr_pool::make_nat(uint64_t value, bool church) {
    return intern(aml_expr{aml_expr::nat{value, church}});
}

const aml_expr* aml_expr_pool::make_integer(int64_t value) {
    return intern(aml_expr{aml_expr::integer{value}});
}

const aml_expr* aml_expr_pool::make_character(uint32_t codepoint) {
    return intern(aml_expr{aml_expr::character{codepoint}});
}

const aml_expr* aml_expr_pool::make_string(std::string value) {
    return intern(aml_expr{aml_expr::string{std::move(value)}});
}

const aml_expr* aml_expr_pool::make_list(std::vector<const aml_expr*> elems, bool church) {
    return intern(aml_expr{aml_expr::list{std::move(elems), church}});
}

const aml_expr* aml_expr_pool::intern(aml_expr&& e) {
    return &*exprs.emplace(std::move(e)).first;
}

size_t aml_expr_pool::size() const {
    return exprs.size();
}
