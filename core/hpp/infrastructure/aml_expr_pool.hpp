#ifndef AML_EXPR_POOL_HPP
#define AML_EXPR_POOL_HPP

#include <set>
#include <string>
#include <vector>
#include "value_objects/aml_expr.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/nat_format.hpp"

struct aml_expr_pool {
    const aml_expr* make_app(const aml_expr* func, const aml_expr* arg);
    const aml_expr* make_abs(std::string param, const aml_expr* body);
    const aml_expr* make_token(std::string name);
    const aml_expr* make_nat(uint64_t value, nat_format format);
    const aml_expr* make_integer(int64_t value);
    const aml_expr* make_character(char value);
    const aml_expr* make_string(std::string value);
    const aml_expr* make_list(std::vector<const aml_expr*> elems, list_format format);

    aml_expr_pool() = default;
    aml_expr_pool(const aml_expr_pool&) = delete;
    aml_expr_pool& operator=(const aml_expr_pool&) = delete;
    aml_expr_pool(aml_expr_pool&&) = default;
    aml_expr_pool& operator=(aml_expr_pool&&) = default;

private:
    const aml_expr* intern(aml_expr&& e);
    std::set<aml_expr> exprs;
};

#endif
