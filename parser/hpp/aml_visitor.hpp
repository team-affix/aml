#ifndef AML_VISITOR_HPP
#define AML_VISITOR_HPP

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "../generated/AMLBaseVisitor.h"
#include "value_objects/aml_expr.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/statement_file.hpp"

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------
//
// Prefer the free functions for file IO:
//   import_module_file(path, pool, pool, …)
//   import_statement_file(path, pool, pool, …)
// Direct visitor use is for in-memory / already-parsed trees:
//   aml_visitor v{pool, pool, …};
//   module_file    mod  = v.parse_module_file(parser.moduleFile());
//   statement_file data = v.parse_statement_file(parser.statementFile());

template<typename IMakeAmlAbs,
         typename IMakeAmlApp,
         typename IMakeAmlSymbol,
         typename IMakeAmlNat,
         typename IMakeAmlInteger,
         typename IMakeAmlCharacter,
         typename IMakeAmlString,
         typename IMakeAmlList>
struct aml_visitor : AMLBaseVisitor {
    aml_visitor(IMakeAmlAbs& make_aml_abs,
                IMakeAmlApp& make_aml_app,
                IMakeAmlSymbol& make_aml_symbol,
                IMakeAmlNat& make_aml_nat,
                IMakeAmlInteger& make_aml_integer,
                IMakeAmlCharacter& make_aml_character,
                IMakeAmlString& make_aml_string,
                IMakeAmlList& make_aml_list);

    module_file    parse_module_file(AMLParser::ModuleFileContext*);
    statement_file parse_statement_file(AMLParser::StatementFileContext*);

private:
    IMakeAmlAbs&       make_aml_abs_;
    IMakeAmlApp&       make_aml_app_;
    IMakeAmlSymbol&    make_aml_symbol_;
    IMakeAmlNat&       make_aml_nat_;
    IMakeAmlInteger&   make_aml_integer_;
    IMakeAmlCharacter& make_aml_character_;
    IMakeAmlString&    make_aml_string_;
    IMakeAmlList&      make_aml_list_;

    declaration_group group_from(AMLParser::DeclarationGroupContext*);
    declaration       decl_from(AMLParser::DeclarationContext*);
    definition        definition_from(AMLParser::DefinitionContext*);
    statement         statement_from(AMLParser::StatementContext*);

    const aml_expr* expr_from(AMLParser::ExprContext*);
    const aml_expr* abs_from(AMLParser::AbstractionContext*);
    const aml_expr* app_from(AMLParser::ApplicationContext*);
    const aml_expr* head_from(AMLParser::HeadContext*);
    const aml_expr* arg_from(AMLParser::ArgContext*);
    const aml_expr* atom_from(AMLParser::AtomContext*);
    const aml_expr* nat_from(AMLParser::NatContext*);
    const aml_expr* int_from(AMLParser::IntContext*);
    const aml_expr* list_from(AMLParser::ListContext*);
    static nat_format  nat_format_from(antlr4::tree::TerminalNode*);
    static list_format list_format_from(antlr4::tree::TerminalNode*);

    static uint64_t    parse_natlit(const std::string&);
    static int64_t     parse_posint(const std::string&);
    static int64_t     parse_negintlit(const std::string&);
    static char        parse_charlit(const std::string&);
    static std::string parse_strlit(const std::string&);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::aml_visitor(
        IAbs& make_aml_abs,
        IApp& make_aml_app,
        ISym& make_aml_symbol,
        INat& make_aml_nat,
        IInt& make_aml_integer,
        IChar& make_aml_character,
        IStr& make_aml_string,
        IList& make_aml_list)
    : make_aml_abs_(make_aml_abs)
    , make_aml_app_(make_aml_app)
    , make_aml_symbol_(make_aml_symbol)
    , make_aml_nat_(make_aml_nat)
    , make_aml_integer_(make_aml_integer)
    , make_aml_character_(make_aml_character)
    , make_aml_string_(make_aml_string)
    , make_aml_list_(make_aml_list) {}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline module_file
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_module_file(
        AMLParser::ModuleFileContext* ctx) {
    module_file file;
    for (auto* item : ctx->moduleItem()) {
        if (auto* dg = item->declarationGroup())
            file.items.push_back(global{group_from(dg)});
        else
            file.items.push_back(global{definition_from(item->definition())});
    }
    return file;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline statement_file
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_statement_file(
        AMLParser::StatementFileContext* ctx) {
    statement_file file;
    for (auto* stmt : ctx->statement())
        file.statements.push_back(statement_from(stmt));
    return file;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline statement
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::statement_from(
        AMLParser::StatementContext* ctx) {
    return {.lhs = expr_from(ctx->expr(0)), .rhs = expr_from(ctx->expr(1))};
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline declaration_group
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::group_from(
        AMLParser::DeclarationGroupContext* ctx) {
    declaration_group group;
    for (auto* d : ctx->declaration())
        group.declarations.push_back(decl_from(d));
    return group;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline declaration
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::decl_from(
        AMLParser::DeclarationContext* ctx) {
    return {ctx->NAME()->getText(),
            static_cast<uint32_t>(std::stoul(ctx->NATLIT()->getText()))};
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline definition
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::definition_from(
        AMLParser::DefinitionContext* ctx) {
    return {ctx->NAME()->getText(), expr_from(ctx->expr())};
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::expr_from(
        AMLParser::ExprContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* app = ctx->application()) return app_from(app);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::abs_from(
        AMLParser::AbstractionContext* ctx) {
    return make_aml_abs_.make_abs(ctx->NAME()->getText(), expr_from(ctx->expr()));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::app_from(
        AMLParser::ApplicationContext* ctx) {
    const aml_expr* result = head_from(ctx->head());
    for (auto* a : ctx->arg())
        result = make_aml_app_.make_app(result, arg_from(a));
    return result;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::head_from(
        AMLParser::HeadContext* ctx) {
    if (auto* inner = ctx->expr()) return expr_from(inner);
    return atom_from(ctx->atom());
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::arg_from(
        AMLParser::ArgContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::atom_from(
        AMLParser::AtomContext* ctx) {
    if (auto* n  = ctx->NAME())    return make_aml_symbol_.make_symbol(n->getText());
    if (auto* na = ctx->nat())     return nat_from(na);
    if (auto* i  = ctx->int_())    return int_from(i);
    if (auto* cl = ctx->CHARLIT())
        return make_aml_character_.make_character(parse_charlit(cl->getText()));
    if (auto* sl = ctx->STRLIT())
        return make_aml_string_.make_string(parse_strlit(sl->getText()));
    assert(ctx->list());
    return list_from(ctx->list());
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::nat_from(
        AMLParser::NatContext* ctx) {
    return make_aml_nat_.make_nat(parse_natlit(ctx->NATLIT()->getText()),
                                  nat_format_from(ctx->ENCODING()));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::int_from(
        AMLParser::IntContext* ctx) {
    int64_t value = ctx->POSINTLIT() ? parse_posint(ctx->POSINTLIT()->getText())
                                     : parse_negintlit(ctx->NEGINTLIT()->getText());
    return make_aml_integer_.make_integer(value, nat_format_from(ctx->ENCODING()));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline const aml_expr*
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::list_from(
        AMLParser::ListContext* ctx) {
    std::vector<const aml_expr*> elems;
    for (auto* e : ctx->expr())
        elems.push_back(expr_from(e));
    return make_aml_list_.make_list(std::move(elems), list_format_from(ctx->ENCODING()));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline nat_format
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::nat_format_from(
        antlr4::tree::TerminalNode* enc) {
    if (!enc) return nat_format::binary;
    const std::string name = enc->getText().substr(1, enc->getText().size() - 2);
    if (name == "church") return nat_format::church;
    return nat_format::binary;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline list_format
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::list_format_from(
        antlr4::tree::TerminalNode* enc) {
    if (!enc) return list_format::scott;
    const std::string name = enc->getText().substr(1, enc->getText().size() - 2);
    if (name == "church") return list_format::church;
    return list_format::scott;
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline uint64_t
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_natlit(
        const std::string& text) {
    return std::stoull(text);
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline int64_t
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_posint(
        const std::string& text) {
    return static_cast<int64_t>(std::stoull(text.substr(1)));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline int64_t
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_negintlit(
        const std::string& text) {
    return -static_cast<int64_t>(std::stoull(text.substr(1)));
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline char
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_charlit(
        const std::string& text) {
    assert(text.size() >= 3 && text.front() == '\'' && text.back() == '\'');
    if (text.at(1) == '\\') {
        switch (text.at(2)) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            default:   return text.at(2);
        }
    }
    return text.at(1);
}

template<typename IAbs, typename IApp, typename ISym, typename INat,
         typename IInt, typename IChar, typename IStr, typename IList>
inline std::string
aml_visitor<IAbs, IApp, ISym, INat, IInt, IChar, IStr, IList>::parse_strlit(
        const std::string& text) {
    assert(text.size() >= 2 && text.front() == '"' && text.back() == '"');
    std::string result;
    for (size_t i = 1; i + 1 < text.size(); ++i) {
        if (text.at(i) == '\\' && i + 2 < text.size()) {
            ++i;
            switch (text.at(i)) {
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"':  result += '"';  break;
                default:   result += text.at(i); break;
            }
        } else {
            result += text.at(i);
        }
    }
    return result;
}

#endif
