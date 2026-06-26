#ifndef EXPR_TRANSPILER_HPP
#define EXPR_TRANSPILER_HPP

#include <stdexcept>
#include <variant>
#include <vector>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/name_env.hpp"
#include "infrastructure/scott_encoder.hpp"
#include "value_objects/aml_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcLam, typename IMakeLcApp>
struct expr_transpiler {
    expr_transpiler(IMakeLcVar& make_var, IMakeLcLam& make_lam, IMakeLcApp& make_app);

    const lc_expr* transpile_expr(const aml_expr* e, const local_binding_env& local,
                                  const global_env& global);

private:
    scott_encoder<IMakeLcVar, IMakeLcLam, IMakeLcApp> encoder_;
    IMakeLcVar& make_var_;
    IMakeLcLam& make_lam_;
    IMakeLcApp& make_app_;

    const lc_expr* resolve_name(const std::string& name, const local_binding_env& local,
                                const global_env& global);
    const lc_expr* transpile_var(const aml_expr::var& v, const local_binding_env& local,
                                 const global_env& global);
    const lc_expr* transpile_abs(const aml_expr::abs& a, const local_binding_env& local,
                                 const global_env& global);
    const lc_expr* transpile_app(const aml_expr::app& a, const local_binding_env& local,
                                 const global_env& global);
    const lc_expr* transpile_nat(const aml_expr::nat& n, const local_binding_env& local,
                                 const global_env& global);
    const lc_expr* transpile_integer(const aml_expr::integer& i, const local_binding_env& local,
                                     const global_env& global);
    const lc_expr* transpile_character(const aml_expr::character& c,
                                       const local_binding_env& local,
                                       const global_env& global);
    const lc_expr* transpile_string(const aml_expr::string& s, const local_binding_env& local,
                                    const global_env& global);
    const lc_expr* transpile_list(const aml_expr::list& l, const local_binding_env& local,
                                  const global_env& global);
    const lc_expr* transpile(const aml_expr* e, const local_binding_env& local,
                             const global_env& global);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template<typename IV, typename IL, typename IA>
expr_transpiler<IV, IL, IA>::expr_transpiler(IV& make_var, IL& make_lam, IA& make_app)
    : encoder_(make_var, make_lam, make_app),
      make_var_(make_var),
      make_lam_(make_lam),
      make_app_(make_app) {}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::resolve_name(const std::string& name,
                                                          const local_binding_env& local,
                                                          const global_env& global) {
    if (auto idx = local.lookup_local(name))
        return make_var_.make_var(*idx);
    if (auto k = global.lookup_global(name))
        return make_var_.make_var(local.depth() + *k);
    throw std::runtime_error("unbound name: " + name);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_expr(const aml_expr* e,
                                                             const local_binding_env& local,
                                                             const global_env& global) {
    return transpile(e, local, global);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_var(const aml_expr::var& v,
                                                           const local_binding_env& local,
                                                           const global_env& global) {
    return resolve_name(v.name, local, global);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_abs(const aml_expr::abs& a,
                                                           const local_binding_env& local,
                                                           const global_env& global) {
    local_binding_env inner = local;
    inner.push(a.param);
    return make_lam_.make_lam(transpile(a.body, inner, global));
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_app(const aml_expr::app& a,
                                                           const local_binding_env& local,
                                                           const global_env& global) {
    return make_app_.make_app(transpile(a.func, local, global),
                              transpile(a.arg, local, global));
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_nat(const aml_expr::nat& n,
                                                           const local_binding_env& local,
                                                           const global_env& global) {
    return n.church ? encoder_.church_nat(n.value)
                      : encoder_.scott_nat(n.value, global);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_integer(const aml_expr::integer& i,
                                                                const local_binding_env& local,
                                                                const global_env& global) {
    return encoder_.scott_integer(i.value, global, local);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_character(const aml_expr::character& c,
                                                                 const local_binding_env& local,
                                                                 const global_env& global) {
    return encoder_.scott_integer(static_cast<int64_t>(c.codepoint), global, local);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_string(const aml_expr::string& s,
                                                              const local_binding_env& local,
                                                              const global_env& global) {
    return encoder_.scott_string(s.value, global, local);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile_list(const aml_expr::list& l,
                                                            const local_binding_env& local,
                                                            const global_env& global) {
    std::vector<const lc_expr*> elems;
    elems.reserve(l.elems.size());
    for (const aml_expr* e : l.elems)
        elems.push_back(transpile(e, local, global));
    return l.church ? encoder_.church_list(elems)
                    : encoder_.scott_list(elems, global);
}

template<typename IV, typename IL, typename IA>
const lc_expr* expr_transpiler<IV, IL, IA>::transpile(const aml_expr* e,
                                                       const local_binding_env& local,
                                                       const global_env& global) {
    if (const auto* v = std::get_if<aml_expr::var>(&e->content))
        return transpile_var(*v, local, global);
    if (const auto* a = std::get_if<aml_expr::abs>(&e->content))
        return transpile_abs(*a, local, global);
    if (const auto* ap = std::get_if<aml_expr::app>(&e->content))
        return transpile_app(*ap, local, global);
    if (const auto* n = std::get_if<aml_expr::nat>(&e->content))
        return transpile_nat(*n, local, global);
    if (const auto* i = std::get_if<aml_expr::integer>(&e->content))
        return transpile_integer(*i, local, global);
    if (const auto* c = std::get_if<aml_expr::character>(&e->content))
        return transpile_character(*c, local, global);
    if (const auto* s = std::get_if<aml_expr::string>(&e->content))
        return transpile_string(*s, local, global);
    if (const auto* l = std::get_if<aml_expr::list>(&e->content))
        return transpile_list(*l, local, global);
    throw std::runtime_error("unsupported aml_expr variant");
}

inline global_env global_env_from_builtin_names() {
    std::vector<std::string> names;
    names.reserve(builtin_constructor_names.size());
    for (std::string_view name : builtin_constructor_names)
        names.emplace_back(name);
    return global_env(std::move(names));
}

#endif
