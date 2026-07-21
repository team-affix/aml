#ifndef BUILTIN_TRANSPILER_HPP
#define BUILTIN_TRANSPILER_HPP

#include <string>
#include "value_objects/bool_decl_group.hpp"
#include "value_objects/int_decl_group.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/nat_decl_group.hpp"

template<typename ITranspileDecl>
struct builtin_transpiler {
    builtin_transpiler(ITranspileDecl& transpile_decl);

    const lc_expr* transpile_true();
    const lc_expr* transpile_false();
    const lc_expr* transpile_cons();
    const lc_expr* transpile_nil();
    const lc_expr* transpile_suc();
    const lc_expr* transpile_zero();
    const lc_expr* transpile_pos();
    const lc_expr* transpile_negsuc();

    // nullptr if name is not a builtin ctor.
    const lc_expr* try_transpile_name(const std::string& name);

private:
    ITranspileDecl& transpile_decl_;
};

template<typename ITD>
builtin_transpiler<ITD>::builtin_transpiler(ITD& transpile_decl)
    : transpile_decl_(transpile_decl) {}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_true() {
    return transpile_decl_.transpile_decl(2u, 0u, 0u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_false() {
    return transpile_decl_.transpile_decl(2u, 1u, 0u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_cons() {
    return transpile_decl_.transpile_decl(2u, 0u, 2u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_nil() {
    return transpile_decl_.transpile_decl(2u, 1u, 0u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_suc() {
    return transpile_decl_.transpile_decl(2u, 0u, 1u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_zero() {
    return transpile_decl_.transpile_decl(2u, 1u, 0u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_pos() {
    return transpile_decl_.transpile_decl(2u, 0u, 1u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::transpile_negsuc() {
    return transpile_decl_.transpile_decl(2u, 1u, 1u);
}

template<typename ITD>
const lc_expr* builtin_transpiler<ITD>::try_transpile_name(const std::string& name) {
    if (name == k_true_name)
        return transpile_true();
    if (name == k_false_name)
        return transpile_false();
    if (name == k_cons_name)
        return transpile_cons();
    if (name == k_nil_name)
        return transpile_nil();
    if (name == k_suc_name)
        return transpile_suc();
    if (name == k_zero_name)
        return transpile_zero();
    if (name == k_pos_name)
        return transpile_pos();
    if (name == k_negsuc_name)
        return transpile_negsuc();
    return nullptr;
}

#endif
