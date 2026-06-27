#ifndef TRANSPILER_HPP
#define TRANSPILER_HPP

#include <stdexcept>
#include <variant>
#include "infrastructure/abs_transpiler.hpp"
#include "infrastructure/app_transpiler.hpp"
#include "infrastructure/character_transpiler.hpp"
#include "infrastructure/church_list_transpiler.hpp"
#include "infrastructure/church_nat_transpiler.hpp"
#include "infrastructure/integer_transpiler.hpp"
#include "infrastructure/scott_list_transpiler.hpp"
#include "infrastructure/scott_nat_transpiler.hpp"
#include "infrastructure/string_transpiler.hpp"
#include "infrastructure/token_transpiler.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp,
         typename IGetVarIndex, typename IPushVar, typename IPopVar>
struct transpiler {
    transpiler(IMakeLcVar& make_var, IMakeLcAbs& make_abs, IMakeLcApp& make_app,
               IGetVarIndex& get_var_index, IPushVar& push_var, IPopVar& pop_var);

    const lc_expr* transpile(const aml_expr* e);

private:
    using self = transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp, IGetVarIndex, IPushVar, IPopVar>;

    token_transpiler<IMakeLcVar, IGetVarIndex>                          token_;
    scott_nat_transpiler<IMakeLcVar, IMakeLcApp, IGetVarIndex>          scott_nat_;
    church_nat_transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp>           church_nat_;
    integer_transpiler<IMakeLcVar, IMakeLcApp,
                       scott_nat_transpiler<IMakeLcVar, IMakeLcApp, IGetVarIndex>,
                       IGetVarIndex>                                     integer_;
    character_transpiler<decltype(scott_nat_)>                          character_;
    scott_list_transpiler<self, IMakeLcVar, IMakeLcApp, IGetVarIndex>   scott_list_;
    church_list_transpiler<self, IMakeLcVar, IMakeLcAbs, IMakeLcApp>    church_list_;
    string_transpiler<decltype(scott_nat_), IMakeLcVar, IMakeLcApp,
                      IGetVarIndex>                                      string_;
    abs_transpiler<self, IMakeLcAbs, IPushVar, IPopVar>                 abs_;
    app_transpiler<self, IMakeLcApp>                                    app_;
};

template<typename IV, typename IL, typename IA, typename IG, typename IP, typename IO>
transpiler<IV, IL, IA, IG, IP, IO>::transpiler(IV& make_var, IL& make_abs, IA& make_app,
                                               IG& get_var_index, IP& push_var, IO& pop_var)
    : token_(make_var, get_var_index),
      scott_nat_(make_var, make_app, get_var_index),
      church_nat_(make_var, make_abs, make_app),
      integer_(make_var, make_app, scott_nat_, get_var_index),
      character_(scott_nat_),
      scott_list_(*this, make_var, make_app, get_var_index),
      church_list_(*this, make_var, make_abs, make_app),
      string_(scott_nat_, make_var, make_app, get_var_index),
      abs_(*this, make_abs, push_var, pop_var),
      app_(*this, make_app) {}

template<typename IV, typename IL, typename IA, typename IG, typename IP, typename IO>
const lc_expr* transpiler<IV, IL, IA, IG, IP, IO>::transpile(const aml_expr* e) {
    if (const auto* t = std::get_if<aml_expr::token>(&e->content))
        return token_.transpile_token(*t);
    if (const auto* a = std::get_if<aml_expr::abs>(&e->content))
        return abs_.transpile_abs(*a);
    if (const auto* ap = std::get_if<aml_expr::app>(&e->content))
        return app_.transpile_app(*ap);
    if (const auto* n = std::get_if<aml_expr::nat>(&e->content)) {
        switch (n->format) {
            case nat_format::scott:  return scott_nat_.transpile_nat(*n);
            case nat_format::church: return church_nat_.transpile_nat(*n);
        }
        throw std::runtime_error("unsupported nat_format");
    }
    if (const auto* i = std::get_if<aml_expr::integer>(&e->content))
        return integer_.transpile_integer(*i);
    if (const auto* c = std::get_if<aml_expr::character>(&e->content))
        return character_.transpile_character(*c);
    if (const auto* s = std::get_if<aml_expr::string>(&e->content))
        return string_.transpile_string(*s);
    if (const auto* l = std::get_if<aml_expr::list>(&e->content)) {
        switch (l->format) {
            case list_format::scott:  return scott_list_.transpile_list(*l);
            case list_format::church: return church_list_.transpile_list(*l);
        }
        throw std::runtime_error("unsupported list_format");
    }
    throw std::runtime_error("unsupported aml_expr variant");
}

#endif
