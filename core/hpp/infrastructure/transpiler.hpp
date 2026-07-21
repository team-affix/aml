#ifndef TRANSPILER_HPP
#define TRANSPILER_HPP

#include <stdexcept>
#include <variant>
#include "infrastructure/abs_transpiler.hpp"
#include "infrastructure/app_transpiler.hpp"
#include "infrastructure/character_transpiler.hpp"
#include "infrastructure/integer_transpiler.hpp"
#include "infrastructure/list_transpiler.hpp"
#include "infrastructure/nat_transpiler.hpp"
#include "infrastructure/string_transpiler.hpp"
#include "infrastructure/symbol_transpiler.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar, typename IMakeLcAbs, typename IMakeLcApp,
         typename IGetVarIndex, typename IPushVar, typename IPopVar>
struct transpiler {
    using self = transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp, IGetVarIndex, IPushVar, IPopVar>;

    using symbol_transpiler_t    = symbol_transpiler<IMakeLcVar, IGetVarIndex>;
    using abs_transpiler_t       = abs_transpiler<self, IMakeLcAbs, IPushVar, IPopVar>;
    using app_transpiler_t       = app_transpiler<self, IMakeLcApp>;
    using nat_transpiler_t       = nat_transpiler<IMakeLcVar, IMakeLcAbs, IMakeLcApp, IGetVarIndex>;
    using integer_transpiler_t   = integer_transpiler<IMakeLcVar, IMakeLcApp, nat_transpiler_t, IGetVarIndex>;
    using character_transpiler_t = character_transpiler<nat_transpiler_t>;
    using string_transpiler_t    = string_transpiler<nat_transpiler_t, IMakeLcVar, IMakeLcApp, IGetVarIndex>;
    using list_transpiler_t      = list_transpiler<self, IMakeLcVar, IMakeLcAbs, IMakeLcApp, IGetVarIndex>;

    transpiler(symbol_transpiler_t&,
               abs_transpiler_t&, app_transpiler_t&,
               nat_transpiler_t&,
               integer_transpiler_t&, character_transpiler_t&,
               string_transpiler_t&,
               list_transpiler_t&);

    const lc_expr* transpile(const aml_expr* e);

private:
    symbol_transpiler_t&    symbol_;
    abs_transpiler_t&       abs_;
    app_transpiler_t&       app_;
    nat_transpiler_t&       nat_;
    integer_transpiler_t&   integer_;
    character_transpiler_t& character_;
    string_transpiler_t&    string_;
    list_transpiler_t&      list_;
};

template<typename IV, typename IL, typename IA, typename IG, typename IP, typename IO>
transpiler<IV, IL, IA, IG, IP, IO>::transpiler(symbol_transpiler_t& symbol,
                                               abs_transpiler_t& abs, app_transpiler_t& app,
                                               nat_transpiler_t& nat,
                                               integer_transpiler_t& integer,
                                               character_transpiler_t& character,
                                               string_transpiler_t& string,
                                               list_transpiler_t& list)
    : symbol_(symbol)
    , abs_(abs)
    , app_(app)
    , nat_(nat)
    , integer_(integer)
    , character_(character)
    , string_(string)
    , list_(list) {}

template<typename IV, typename IL, typename IA, typename IG, typename IP, typename IO>
const lc_expr* transpiler<IV, IL, IA, IG, IP, IO>::transpile(const aml_expr* e) {
    if (const auto* t = std::get_if<aml_expr::symbol>(&e->content))
        return symbol_.transpile_symbol(*t);
    if (const auto* a = std::get_if<aml_expr::abs>(&e->content))
        return abs_.transpile_abs(*a);
    if (const auto* ap = std::get_if<aml_expr::app>(&e->content))
        return app_.transpile_app(*ap);
    if (const auto* n = std::get_if<aml_expr::nat>(&e->content))
        return nat_.transpile_nat(*n);
    if (const auto* i = std::get_if<aml_expr::integer>(&e->content))
        return integer_.transpile_integer(*i);
    if (const auto* c = std::get_if<aml_expr::character>(&e->content))
        return character_.transpile_character(*c);
    if (const auto* s = std::get_if<aml_expr::string>(&e->content))
        return string_.transpile_string(*s);
    if (const auto* l = std::get_if<aml_expr::list>(&e->content))
        return list_.transpile_list(*l);
    throw std::runtime_error("unsupported aml_expr variant");
}

#endif
