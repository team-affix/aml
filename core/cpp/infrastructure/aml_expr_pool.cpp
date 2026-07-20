#include "infrastructure/aml_expr_pool.hpp"

const aml_expr* aml_expr_pool::make_app(const aml_expr* func, const aml_expr* arg) {
    return intern(aml_expr{aml_expr::app{func, arg}});
}

const aml_expr* aml_expr_pool::make_abs(std::string param, const aml_expr* body) {
    return intern(aml_expr{aml_expr::abs{std::move(param), body}});
}

const aml_expr* aml_expr_pool::make_symbol(std::string name) {
    return intern(aml_expr{aml_expr::symbol{std::move(name)}});
}

const aml_expr* aml_expr_pool::make_nat(uint64_t value, nat_format format) {
    return intern(aml_expr{aml_expr::nat{value, format}});
}

const aml_expr* aml_expr_pool::make_integer(int64_t value, nat_format format) {
    return intern(aml_expr{aml_expr::integer{value, format}});
}

const aml_expr* aml_expr_pool::make_character(char value) {
    return intern(aml_expr{aml_expr::character{value}});
}

const aml_expr* aml_expr_pool::make_string(std::string value) {
    return intern(aml_expr{aml_expr::string{std::move(value)}});
}

const aml_expr* aml_expr_pool::make_list(std::vector<const aml_expr*> elems, list_format format) {
    return intern(aml_expr{aml_expr::list{std::move(elems), format}});
}

const aml_expr* aml_expr_pool::intern(aml_expr&& e) {
    return &*exprs.emplace(std::move(e)).first;
}
