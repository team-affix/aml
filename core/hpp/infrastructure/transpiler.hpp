#ifndef TRANSPILER_HPP
#define TRANSPILER_HPP

#include <stdexcept>
#include <variant>
#include "infrastructure/abs_transpiler.hpp"
#include "infrastructure/app_transpiler.hpp"
#include "infrastructure/church_list_transpiler.hpp"
#include "infrastructure/church_nat_transpiler.hpp"
#include "infrastructure/character_transpiler.hpp"
#include "infrastructure/global_env.hpp"
#include "infrastructure/integer_transpiler.hpp"
#include "infrastructure/local_binding_env.hpp"
#include "infrastructure/scott_list_transpiler.hpp"
#include "infrastructure/scott_nat_transpiler.hpp"
#include "infrastructure/string_transpiler.hpp"
#include "infrastructure/token_transpiler.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp>
struct transpiler {
    explicit transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app);

    const lc_expr* transpile(const aml_expr* e, const local_binding_env& local,
                             const global_env& global);

private:
    using self = transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp>;

    token_transpiler<IMakeLcVar> token_;
    scott_nat_transpiler<IMakeLcVar, IMakeLcApp> scott_nat_;
    church_nat_transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp> church_nat_;
    integer_transpiler<IMakeLcVar, IMakeLcApp, scott_nat_transpiler<IMakeLcVar, IMakeLcApp>>
        integer_;
    character_transpiler<decltype(scott_nat_)> character_;
    scott_list_transpiler<self, IMakeLcVar, IMakeLcApp> scott_list_;
    church_list_transpiler<self, IMakeLcVar, IMakeLcAbs, IMakeLcApp> church_list_;
    string_transpiler<decltype(scott_nat_), IMakeLcVar, IMakeLcApp> string_;
    abs_transpiler<self, IMakeLcAbs> abs_;
    app_transpiler<self, IMakeLcApp> app_;
};

template<typename IV, typename IL, typename IA>
transpiler<IV, IL, IA>::transpiler(IV& make_var, IL& make_abs, IA& make_app)
    : token_(make_var),
      scott_nat_(make_var, make_app),
      church_nat_(make_var, make_abs, make_app),
      integer_(make_var, make_app, scott_nat_),
      character_(scott_nat_),
      scott_list_(*this, make_var, make_app),
      church_list_(*this, make_var, make_abs, make_app),
      string_(scott_nat_, make_var, make_app),
      abs_(*this, make_abs),
      app_(*this, make_app) {}

template<typename IV, typename IL, typename IA>
const lc_expr* transpiler<IV, IL, IA>::transpile(const aml_expr* e, const local_binding_env& local,
                                                const global_env& global) {
    if (const auto* t = std::get_if<aml_expr::token>(&e->content))
        return token_.transpile_token(*t, local, global);
    if (const auto* a = std::get_if<aml_expr::abs>(&e->content))
        return abs_.transpile_abs(*a, local, global);
    if (const auto* ap = std::get_if<aml_expr::app>(&e->content))
        return app_.transpile_app(*ap, local, global);
    if (const auto* n = std::get_if<aml_expr::nat>(&e->content)) {
        switch (n->format) {
            case nat_format::scott:
                return scott_nat_.transpile_nat(*n, local, global);
            case nat_format::church:
                return church_nat_.transpile_nat(*n, local, global);
        }
        throw std::runtime_error("unsupported nat_format");
    }
    if (const auto* i = std::get_if<aml_expr::integer>(&e->content))
        return integer_.transpile_integer(*i, local, global);
    if (const auto* c = std::get_if<aml_expr::character>(&e->content))
        return character_.transpile_character(*c, local, global);
    if (const auto* s = std::get_if<aml_expr::string>(&e->content))
        return string_.transpile_string(*s, local, global);
    if (const auto* l = std::get_if<aml_expr::list>(&e->content)) {
        switch (l->format) {
            case list_format::scott:
                return scott_list_.transpile_list(*l, local, global);
            case list_format::church:
                return church_list_.transpile_list(*l, local, global);
        }
        throw std::runtime_error("unsupported list_format");
    }
    throw std::runtime_error("unsupported aml_expr variant");
}

#endif
