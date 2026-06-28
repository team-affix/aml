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
    using self = transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp, IGetVarIndex, IPushVar, IPopVar>;

    using token_transpiler_t      = token_transpiler<IMakeLcVar, IGetVarIndex>;
    using scott_nat_transpiler_t  = scott_nat_transpiler<IMakeLcVar, IMakeLcApp, IGetVarIndex>;
    using church_nat_transpiler_t = church_nat_transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp>;
    using integer_transpiler_t    = integer_transpiler<IMakeLcVar, IMakeLcApp, scott_nat_transpiler_t, IGetVarIndex>;
    using character_transpiler_t  = character_transpiler<scott_nat_transpiler_t>;
    using string_transpiler_t     = string_transpiler<scott_nat_transpiler_t, IMakeLcVar, IMakeLcApp, IGetVarIndex>;
    using scott_list_transpiler_t = scott_list_transpiler<self, IMakeLcVar, IMakeLcApp, IGetVarIndex>;
    using church_list_transpiler_t = church_list_transpiler<self, IMakeLcVar, IMakeLcAbs, IMakeLcApp>;
    using abs_transpiler_t        = abs_transpiler<self, IMakeLcAbs, IPushVar, IPopVar>;
    using app_transpiler_t        = app_transpiler<self, IMakeLcApp>;

    transpiler(token_transpiler_t&, abs_transpiler_t&, app_transpiler_t&,
               scott_nat_transpiler_t&, church_nat_transpiler_t&,
               integer_transpiler_t&, character_transpiler_t&,
               scott_list_transpiler_t&, church_list_transpiler_t&,
               string_transpiler_t&);

    const lc_expr* transpile(const aml_expr* e);

private:
    token_transpiler_t&      token_;
    abs_transpiler_t&        abs_;
    app_transpiler_t&        app_;
    scott_nat_transpiler_t&  scott_nat_;
    church_nat_transpiler_t& church_nat_;
    integer_transpiler_t&    integer_;
    character_transpiler_t&  character_;
    scott_list_transpiler_t& scott_list_;
    church_list_transpiler_t& church_list_;
    string_transpiler_t&     string_;
};

template<typename IV, typename IL, typename IA, typename IG, typename IP, typename IO>
transpiler<IV, IL, IA, IG, IP, IO>::transpiler(token_transpiler_t& token, abs_transpiler_t& abs, app_transpiler_t& app,
                                               scott_nat_transpiler_t& scott_nat, church_nat_transpiler_t& church_nat,
                                               integer_transpiler_t& integer, character_transpiler_t& character,
                                               scott_list_transpiler_t& scott_list, church_list_transpiler_t& church_list,
                                               string_transpiler_t& string)
    : token_(token),
      abs_(abs),
      app_(app),
      scott_nat_(scott_nat),
      church_nat_(church_nat),
      integer_(integer),
      character_(character),
      scott_list_(scott_list),
      church_list_(church_list),
      string_(string) {}

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
