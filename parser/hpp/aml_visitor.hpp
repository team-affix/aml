#ifndef AML_VISITOR_HPP
#define AML_VISITOR_HPP

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "../generated/AMLBaseVisitor.h"
#include "value_objects/aml_expr.hpp"
#include "value_objects/declaration_file.hpp"
#include "value_objects/definition_file.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/statement_file.hpp"

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------
//
// Usage:
//   aml_expr_pool pool;
//   aml_visitor v{pool};
//   declaration_file decls = v.parse_declaration_file(parser.declarationFile());
//   definition_file  defs  = v.parse_definition_file(parser.definitionFile());
//   statement_file   data   = v.parse_statement_file(parser.statementFile());

template<typename IMakeAml>
struct aml_visitor : AMLBaseVisitor {
    explicit aml_visitor(IMakeAml& make_aml);

    declaration_file parse_declaration_file(AMLParser::DeclarationFileContext*);
    definition_file  parse_definition_file(AMLParser::DefinitionFileContext*);
    statement_file parse_statement_file(AMLParser::StatementFileContext*);

private:
    IMakeAml& make_aml_;

    declaration_group group_from(AMLParser::DeclarationGroupContext*);
    declaration_decl  decl_from(AMLParser::DeclarationContext*);
    definition        definition_from(AMLParser::DefinitionContext*);
    statement         statement_from(AMLParser::StatementContext*);

    const aml_expr* expr_from(AMLParser::ExprContext*);
    const aml_expr* abs_from(AMLParser::AbstractionContext*);
    const aml_expr* app_from(AMLParser::ApplicationContext*);
    const aml_expr* head_from(AMLParser::HeadContext*);
    const aml_expr* arg_from(AMLParser::ArgContext*);
    const aml_expr* atom_from(AMLParser::AtomContext*);
    const aml_expr* encoded_nat_from(AMLParser::EncodedNatContext*);
    const aml_expr* int_lit_from(AMLParser::IntLitContext*);
    const aml_expr* encoded_list_from(AMLParser::EncodedListContext*);
    static nat_format  nat_format_from(AMLParser::EncodingContext*);
    static list_format list_format_from(AMLParser::EncodingContext*);

    static uint64_t    parse_natlit(const std::string&);
    static int64_t     parse_posint(const std::string&);
    static int64_t     parse_negintlit(const std::string&);
    static char        parse_charlit(const std::string&);
    static std::string parse_strlit(const std::string&);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template<typename M>
inline aml_visitor<M>::aml_visitor(M& make_aml) : make_aml_(make_aml) {}

template<typename M>
inline declaration_file aml_visitor<M>::parse_declaration_file(
    AMLParser::DeclarationFileContext* ctx) {
    declaration_file file;
    for (auto* group : ctx->declarationGroup())
        file.groups.push_back(group_from(group));
    return file;
}

template<typename M>
inline definition_file aml_visitor<M>::parse_definition_file(
    AMLParser::DefinitionFileContext* ctx) {
    definition_file file;
    for (auto* def : ctx->definition())
        file.definitions.push_back(definition_from(def));
    return file;
}

template<typename M>
inline statement_file aml_visitor<M>::parse_statement_file(
    AMLParser::StatementFileContext* ctx) {
    statement_file file;
    for (auto* stmt : ctx->statement())
        file.statements.push_back(statement_from(stmt));
    return file;
}

template<typename M>
inline statement aml_visitor<M>::statement_from(AMLParser::StatementContext* ctx) {
    return {expr_from(ctx->expr(0)), expr_from(ctx->expr(1))};
}

template<typename M>
inline declaration_group aml_visitor<M>::group_from(AMLParser::DeclarationGroupContext* ctx) {
    declaration_group group;
    for (auto* d : ctx->declaration())
        group.declarations.push_back(decl_from(d));
    return group;
}

template<typename M>
inline declaration_decl aml_visitor<M>::decl_from(AMLParser::DeclarationContext* ctx) {
    return {ctx->NAME()->getText(),
            static_cast<uint32_t>(std::stoul(ctx->POSINT()->getText()))};
}

template<typename M>
inline definition aml_visitor<M>::definition_from(AMLParser::DefinitionContext* ctx) {
    return {ctx->NAME()->getText(), expr_from(ctx->expr())};
}

template<typename M>
inline const aml_expr* aml_visitor<M>::expr_from(AMLParser::ExprContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* app = ctx->application()) return app_from(app);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename M>
inline const aml_expr* aml_visitor<M>::abs_from(AMLParser::AbstractionContext* ctx) {
    return make_aml_.make_abs(ctx->NAME()->getText(), expr_from(ctx->expr()));
}

template<typename M>
inline const aml_expr* aml_visitor<M>::app_from(AMLParser::ApplicationContext* ctx) {
    const aml_expr* result = head_from(ctx->head());
    for (auto* a : ctx->arg())
        result = make_aml_.make_app(result, arg_from(a));
    return result;
}

template<typename M>
inline const aml_expr* aml_visitor<M>::head_from(AMLParser::HeadContext* ctx) {
    if (auto* inner = ctx->expr()) return expr_from(inner);
    return atom_from(ctx->atom());
}

template<typename M>
inline const aml_expr* aml_visitor<M>::arg_from(AMLParser::ArgContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename M>
inline const aml_expr* aml_visitor<M>::atom_from(AMLParser::AtomContext* ctx) {
    if (auto* n  = ctx->NAME())       return make_aml_.make_token(n->getText());
    if (auto* en = ctx->encodedNat()) return encoded_nat_from(en);
    if (auto* il = ctx->intLit())     return int_lit_from(il);
    if (auto* cl = ctx->CHARLIT())    return make_aml_.make_character(parse_charlit(cl->getText()));
    if (auto* sl = ctx->STRLIT())     return make_aml_.make_string(parse_strlit(sl->getText()));
    assert(ctx->encodedList());
    return encoded_list_from(ctx->encodedList());
}

template<typename M>
inline const aml_expr* aml_visitor<M>::encoded_nat_from(AMLParser::EncodedNatContext* ctx) {
    nat_format format =
        ctx->encoding() ? nat_format_from(ctx->encoding()) : nat_format::scott;
    return make_aml_.make_nat(parse_natlit(ctx->NATLIT()->getText()), format);
}

template<typename M>
inline const aml_expr* aml_visitor<M>::int_lit_from(AMLParser::IntLitContext* ctx) {
    int64_t value = ctx->POSINT() ? parse_posint(ctx->POSINT()->getText())
                                  : parse_negintlit(ctx->NEGINTLIT()->getText());
    return make_aml_.make_integer(value);
}

template<typename M>
inline const aml_expr* aml_visitor<M>::encoded_list_from(AMLParser::EncodedListContext* ctx) {
    list_format format =
        ctx->encoding() ? list_format_from(ctx->encoding()) : list_format::scott;
    std::vector<const aml_expr*> elems;
    for (auto* e : ctx->expr())
        elems.push_back(expr_from(e));
    return make_aml_.make_list(std::move(elems), format);
}

template<typename M>
inline nat_format aml_visitor<M>::nat_format_from(AMLParser::EncodingContext* ctx) {
    return ctx->CHURCH() ? nat_format::church : nat_format::scott;
}

template<typename M>
inline list_format aml_visitor<M>::list_format_from(AMLParser::EncodingContext* ctx) {
    return ctx->CHURCH() ? list_format::church : list_format::scott;
}

template<typename M>
inline uint64_t aml_visitor<M>::parse_natlit(const std::string& text) {
    return std::stoull(text.substr(0, text.size() - 1));
}

template<typename M>
inline int64_t aml_visitor<M>::parse_posint(const std::string& text) {
    return static_cast<int64_t>(std::stoull(text));
}

template<typename M>
inline int64_t aml_visitor<M>::parse_negintlit(const std::string& text) {
    return -static_cast<int64_t>(std::stoull(text.substr(1)));
}

template<typename M>
inline char aml_visitor<M>::parse_charlit(const std::string& text) {
    assert(text.size() >= 3 && text.front() == '\'' && text.back() == '\'');
    if (text[1] == '\\') {
        switch (text[2]) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            default:   return text[2];
        }
    }
    return text[1];
}

template<typename M>
inline std::string aml_visitor<M>::parse_strlit(const std::string& text) {
    assert(text.size() >= 2 && text.front() == '"' && text.back() == '"');
    std::string result;
    for (size_t i = 1; i + 1 < text.size(); ++i) {
        if (text[i] == '\\' && i + 2 < text.size()) {
            ++i;
            switch (text[i]) {
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"':  result += '"';  break;
                default:   result += text[i]; break;
            }
        } else {
            result += text[i];
        }
    }
    return result;
}

#endif
