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
#include "value_objects/training_file.hpp"

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------
//
// Usage:
//   aml_expr_pool pool;
//   aml_visitor v{pool, pool, pool, pool, pool, pool, pool, pool};
//   declaration_file decls = v.parse_declarations(parser.declarationFile());
//   definition_file  defs  = v.parse_definitions(parser.definitionFile());
//   training_file    data  = v.parse_training(parser.trainingFile());

template<typename IMakeAmlApp,
         typename IMakeAmlAbs,
         typename IMakeAmlVar,
         typename IMakeAmlNat,
         typename IMakeAmlInteger,
         typename IMakeAmlCharacter,
         typename IMakeAmlString,
         typename IMakeAmlList>
struct aml_visitor : AMLBaseVisitor {
    aml_visitor(IMakeAmlApp& make_app,
                IMakeAmlAbs& make_abs,
                IMakeAmlVar& make_var,
                IMakeAmlNat& make_nat,
                IMakeAmlInteger& make_integer,
                IMakeAmlCharacter& make_character,
                IMakeAmlString& make_string,
                IMakeAmlList& make_list);

    declaration_file parse_declarations(AMLParser::DeclarationFileContext*);
    definition_file  parse_definitions(AMLParser::DefinitionFileContext*);
    training_file    parse_training(AMLParser::TrainingFileContext*);

private:
    IMakeAmlApp&       make_app_;
    IMakeAmlAbs&       make_abs_;
    IMakeAmlVar&       make_var_;
    IMakeAmlNat&       make_nat_;
    IMakeAmlInteger&   make_integer_;
    IMakeAmlCharacter& make_character_;
    IMakeAmlString&    make_string_;
    IMakeAmlList&      make_list_;

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

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::aml_visitor(
    IA& make_app, IB& make_abs, IV& make_var, IN& make_nat, II& make_integer,
    IC& make_character, IS& make_string, IL& make_list)
    : make_app_(make_app),
      make_abs_(make_abs),
      make_var_(make_var),
      make_nat_(make_nat),
      make_integer_(make_integer),
      make_character_(make_character),
      make_string_(make_string),
      make_list_(make_list) {}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline declaration_file aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_declarations(
    AMLParser::DeclarationFileContext* ctx) {
    declaration_file file;
    for (auto* group : ctx->constructorGroup())
        file.groups.push_back(group_from(group));
    return file;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline definition_file aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_definitions(
    AMLParser::DefinitionFileContext* ctx) {
    definition_file file;
    for (auto* def : ctx->functionDef())
        file.functions.push_back(fndef_from(def));
    return file;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline training_file aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_training(
    AMLParser::TrainingFileContext* ctx) {
    training_file file;
    for (auto* stmt : ctx->statement())
        file.statements.push_back(expr_from(stmt->expr()));
    return file;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline constructor_group aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::group_from(
    AMLParser::ConstructorGroupContext* ctx) {
    constructor_group group;
    for (auto* c : ctx->constructor())
        group.constructors.push_back(decl_from(c));
    return group;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline constructor_decl aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::decl_from(
    AMLParser::ConstructorContext* ctx) {
    return {ctx->NAME()->getText(),
            static_cast<uint32_t>(std::stoul(ctx->POSINT()->getText()))};
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline function_def aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::fndef_from(
    AMLParser::FunctionDefContext* ctx) {
    return {ctx->NAME()->getText(), expr_from(ctx->expr())};
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::expr_from(
    AMLParser::ExprContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* app = ctx->application()) return app_from(app);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::abs_from(
    AMLParser::AbstractionContext* ctx) {
    return make_abs_.make_abs(ctx->NAME()->getText(), expr_from(ctx->expr()));
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::app_from(
    AMLParser::ApplicationContext* ctx) {
    const aml_expr* result = head_from(ctx->head());
    for (auto* a : ctx->arg())
        result = make_app_.make_app(result, arg_from(a));
    return result;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::head_from(
    AMLParser::HeadContext* ctx) {
    if (auto* inner = ctx->expr()) return expr_from(inner);
    return atom_from(ctx->atom());
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::arg_from(
    AMLParser::ArgContext* ctx) {
    if (auto* abs = ctx->abstraction()) return abs_from(abs);
    if (auto* atm = ctx->atom())        return atom_from(atm);
    return expr_from(ctx->expr());
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::atom_from(
    AMLParser::AtomContext* ctx) {
    if (auto* n  = ctx->NAME())       return make_var_.make_var(n->getText());
    if (auto* en = ctx->encodedNat()) return encoded_nat_from(en);
    if (auto* il = ctx->intLit())     return int_lit_from(il);
    if (auto* cl = ctx->CHARLIT())    return make_character_.make_character(parse_charlit(cl->getText()));
    if (auto* sl = ctx->STRLIT())     return make_string_.make_string(parse_strlit(sl->getText()));
    assert(ctx->encodedList());
    return encoded_list_from(ctx->encodedList());
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::encoded_nat_from(
    AMLParser::EncodedNatContext* ctx) {
    bool church = ctx->encoding() && encoding_from(ctx->encoding());
    return make_nat_.make_nat(parse_natlit(ctx->NATLIT()->getText()), church);
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::int_lit_from(
    AMLParser::IntLitContext* ctx) {
    int64_t value = ctx->POSINT() ? parse_posint(ctx->POSINT()->getText())
                                  : parse_negintlit(ctx->NEGINTLIT()->getText());
    return make_integer_.make_integer(value);
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline const aml_expr* aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::encoded_list_from(
    AMLParser::EncodedListContext* ctx) {
    bool church = ctx->encoding() && encoding_from(ctx->encoding());
    std::vector<const aml_expr*> elems;
    for (auto* e : ctx->expr())
        elems.push_back(expr_from(e));
    return make_list_.make_list(std::move(elems), church);
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline bool aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::encoding_from(
    AMLParser::EncodingContext* ctx) {
    return ctx->CHURCH() != nullptr;
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline uint64_t aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_natlit(
    const std::string& text) {
    return std::stoull(text.substr(0, text.size() - 1));
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline int64_t aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_posint(
    const std::string& text) {
    return static_cast<int64_t>(std::stoull(text));
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline int64_t aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_negintlit(
    const std::string& text) {
    return -static_cast<int64_t>(std::stoull(text.substr(1)));
}

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline uint32_t aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_charlit(
    const std::string& text) {
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

template<typename IA, typename IB, typename IV, typename IN, typename II,
         typename IC, typename IS, typename IL>
inline std::string aml_visitor<IA, IB, IV, IN, II, IC, IS, IL>::parse_strlit(
    const std::string& text) {
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
