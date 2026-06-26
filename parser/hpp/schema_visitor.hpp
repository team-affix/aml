#ifndef SCHEMA_VISITOR_HPP
#define SCHEMA_VISITOR_HPP

#include <any>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "../generated/AMLBaseVisitor.h"
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/aml_expr.hpp"

// ---------------------------------------------------------------------------
// Output types
// ---------------------------------------------------------------------------

struct constructor_decl {
    std::string name;
    uint32_t    arity;
};

// One unlabeled group (e.g. true/0 | false/0).
// Position of each constructor within the group determines its eliminator index.
using constructor_group = std::vector<constructor_decl>;

struct function_def {
    std::string     name;
    const aml_expr* body;
};

struct aml_program {
    aml_expr_pool                  pool;
    std::vector<constructor_group> groups;
    std::vector<function_def>      functions;

    aml_program() = default;
    aml_program(const aml_program&) = delete;
    aml_program& operator=(const aml_program&) = delete;
    aml_program(aml_program&&) = default;
    aml_program& operator=(aml_program&&) = default;
};

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------
//
// Usage:
//   schema_visitor v;
//   aml_program prog = v.parse(parser.program());

struct schema_visitor : public AMLBaseVisitor {
    aml_program parse(AMLParser::ProgramContext*);

private:
    aml_expr_pool pool_;

    constructor_group group_from(AMLParser::ConstructorGroupContext*);
    constructor_decl  decl_from(AMLParser::ConstructorContext*);
    function_def      fndef_from(AMLParser::FunctionDefContext*);

    const aml_expr* expr_from(AMLParser::ExprContext*);
    const aml_expr* abs_from(AMLParser::AbstractionContext*);
    const aml_expr* app_from(AMLParser::ApplicationContext*);
    const aml_expr* head_from(AMLParser::HeadContext*);
    const aml_expr* arg_from(AMLParser::ArgContext*);
    const aml_expr* atom_from(AMLParser::AtomContext*);
    const aml_expr* encoded_nat_from(AMLParser::EncodedNatContext*);
    const aml_expr* int_lit_from(AMLParser::IntLitContext*);
    const aml_expr* encoded_list_from(AMLParser::EncodedListContext*);
    bool            encoding_from(AMLParser::EncodingContext*);

    static uint64_t    parse_natlit(const std::string&);
    static int64_t     parse_posint(const std::string&);
    static int64_t     parse_negintlit(const std::string&);
    static uint32_t    parse_charlit(const std::string&);
    static std::string parse_strlit(const std::string&);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

inline aml_program schema_visitor::parse(AMLParser::ProgramContext* ctx) {
    aml_program prog;
    for (auto* decl : ctx->declaration()) {
        if (auto* cg = decl->constructorGroup())
            prog.groups.push_back(group_from(cg));
        else
            prog.functions.push_back(fndef_from(decl->functionDef()));
    }
    prog.pool = std::move(pool_);
    return prog;
}

inline constructor_group schema_visitor::group_from(AMLParser::ConstructorGroupContext* ctx) {
    constructor_group group;
    for (auto* c : ctx->constructor())
        group.push_back(decl_from(c));
    return group;
}

inline constructor_decl schema_visitor::decl_from(AMLParser::ConstructorContext* ctx) {
    return {ctx->NAME()->getText(),
            static_cast<uint32_t>(std::stoul(ctx->POSINT()->getText()))};
}

inline function_def schema_visitor::fndef_from(AMLParser::FunctionDefContext* ctx) {
    return {ctx->NAME()->getText(), expr_from(ctx->expr())};
}

inline const aml_expr* schema_visitor::expr_from(AMLParser::ExprContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* app = ctx->application()) return app_from(app);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr()); // '(' expr ')'
}

inline const aml_expr* schema_visitor::abs_from(AMLParser::AbstractionContext* ctx) {
    return pool_.make_abs(ctx->NAME()->getText(), expr_from(ctx->expr()));
}

inline const aml_expr* schema_visitor::app_from(AMLParser::ApplicationContext* ctx) {
    // Left-fold: f x y z → ((f x) y) z
    const aml_expr* result = head_from(ctx->head());
    for (auto* a : ctx->arg())
        result = pool_.make_app(result, arg_from(a));
    return result;
}

inline const aml_expr* schema_visitor::head_from(AMLParser::HeadContext* ctx) {
    if (auto* inner = ctx->expr()) return expr_from(inner);
    return atom_from(ctx->atom());
}

inline const aml_expr* schema_visitor::arg_from(AMLParser::ArgContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr()); // '(' expr ')'
}

inline const aml_expr* schema_visitor::atom_from(AMLParser::AtomContext* ctx) {
    if (auto* n  = ctx->NAME())       return pool_.make_var(n->getText());
    if (auto* en = ctx->encodedNat()) return encoded_nat_from(en);
    if (auto* il = ctx->intLit())     return int_lit_from(il);
    if (auto* cl = ctx->CHARLIT())    return pool_.make_character(parse_charlit(cl->getText()));
    if (auto* sl = ctx->STRLIT())     return pool_.make_string(parse_strlit(sl->getText()));
    assert(ctx->encodedList());
    return encoded_list_from(ctx->encodedList());
}

inline const aml_expr* schema_visitor::encoded_nat_from(AMLParser::EncodedNatContext* ctx) {
    bool church = ctx->encoding() && encoding_from(ctx->encoding());
    return pool_.make_nat(parse_natlit(ctx->NATLIT()->getText()), church);
}

inline const aml_expr* schema_visitor::int_lit_from(AMLParser::IntLitContext* ctx) {
    int64_t value = ctx->POSINT() ? parse_posint(ctx->POSINT()->getText())
                                  : parse_negintlit(ctx->NEGINTLIT()->getText());
    return pool_.make_integer(value);
}

inline const aml_expr* schema_visitor::encoded_list_from(AMLParser::EncodedListContext* ctx) {
    bool church = ctx->encoding() && encoding_from(ctx->encoding());
    std::vector<const aml_expr*> elems;
    for (auto* e : ctx->expr())
        elems.push_back(expr_from(e));
    return pool_.make_list(std::move(elems), church);
}

inline bool schema_visitor::encoding_from(AMLParser::EncodingContext* ctx) {
    return ctx->CHURCH() != nullptr; // true = church, false = scott
}

inline uint64_t schema_visitor::parse_natlit(const std::string& text) {
    return std::stoull(text.substr(0, text.size() - 1));
}

inline int64_t schema_visitor::parse_posint(const std::string& text) {
    return static_cast<int64_t>(std::stoull(text));
}

inline int64_t schema_visitor::parse_negintlit(const std::string& text) {
    return -static_cast<int64_t>(std::stoull(text.substr(1)));
}

inline uint32_t schema_visitor::parse_charlit(const std::string& text) {
    assert(text.size() >= 3 && text.front() == '\'' && text.back() == '\'');
    if (text[1] == '\\') {
        switch (text[2]) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            default:   return static_cast<uint32_t>(text[2]);
        }
    }
    return static_cast<uint32_t>(text[1]);
}

inline std::string schema_visitor::parse_strlit(const std::string& text) {
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
