#ifndef GLOBAL_PROCESSOR_HPP
#define GLOBAL_PROCESSOR_HPP

#include <cstdint>
#include <variant>
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr,
         typename ITranspileDecl,
         typename IPushGlobal,
         typename IPushScope>
struct global_processor {
    global_processor(ITranspileExpr& transpile_expr,
                     ITranspileDecl& transpile_decl,
                     IPushGlobal& push_global,
                     IPushScope& push_scope);

    void process_global(const global& item);

private:
    void process_definition(const definition& def);
    void process_declaration(const declaration_group& group);

    ITranspileExpr&  transpile_expr_;
    ITranspileDecl&  transpile_decl_;
    IPushGlobal&     push_global_;
    IPushScope&      push_scope_;
};

template<typename ITE, typename ITD, typename IPG, typename IPS>
global_processor<ITE, ITD, IPG, IPS>::global_processor(
        ITE& transpile_expr,
        ITD& transpile_decl,
        IPG& push_global,
        IPS& push_scope)
    : transpile_expr_(transpile_expr)
    , transpile_decl_(transpile_decl)
    , push_global_(push_global)
    , push_scope_(push_scope) {}

template<typename ITE, typename ITD, typename IPG, typename IPS>
void global_processor<ITE, ITD, IPG, IPS>::process_global(const global& item) {
    if (const definition* def = std::get_if<definition>(&item))
        process_definition(*def);
    else
        process_declaration(std::get<declaration_group>(item));
}

template<typename ITE, typename ITD, typename IPG, typename IPS>
void global_processor<ITE, ITD, IPG, IPS>::process_definition(
        const definition& def) {
    const lc_expr* term = transpile_expr_.transpile(def.body);
    push_global_.push(term);
    push_scope_.push(def.name);
}

template<typename ITE, typename ITD, typename IPG, typename IPS>
void global_processor<ITE, ITD, IPG, IPS>::process_declaration(
        const declaration_group& group) {
    const auto n = static_cast<uint32_t>(group.declarations.size());
    for (uint32_t k = 0; k < n; ++k) {
        const declaration& d = group.declarations.at(k);
        const lc_expr* term = transpile_decl_.transpile_decl(n, k, d.arity);
        push_global_.push(term);
        push_scope_.push(d.name);
    }
}

#endif
