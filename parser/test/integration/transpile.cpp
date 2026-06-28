// End-to-end: parse AML module fragments then transpile to lc_expr.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/transpiler_manifest.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/aml_visitor.hpp"

namespace {

static const std::vector<std::string> kBuiltinNames = {
    "true", "false", "cons", "nil", "pos", "negsuc"
};

aml_expr_pool& shared_pool() {
    static aml_expr_pool pool;
    return pool;
}

aml_visitor<aml_expr_pool> make_visitor() {
    return aml_visitor<aml_expr_pool>{shared_pool()};
}

module_file parse_module(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    return make_visitor().parse_module_file(parser.moduleFile());
}

std::vector<definition> definitions_from(const module_file& mf) {
    std::vector<definition> result;
    for (const auto& item : mf.items)
        if (const auto* d = std::get_if<definition>(&item.content))
            result.push_back(*d);
    return result;
}

const lc_expr* lc_abs(lc_expr_pool& pool, const lc_expr* b) { return pool.make_abs(b); }
const lc_expr* lc_var(lc_expr_pool& pool, uint32_t i)       { return pool.make_var(i); }

void push_names(transpiler_manifest& b, const std::vector<std::string>& names) {
    for (const auto& n : names) b.sc.push(n);
}
void pop_names(transpiler_manifest& b, size_t count) {
    for (size_t i = 0; i < count; ++i) b.sc.pop();
}

} // namespace

TEST(ParseTranspileTest, IdentityFunction) {
    auto ds = definitions_from(parse_module("id = x => x."));
    ASSERT_EQ(ds.size(), 1u);
    EXPECT_EQ(ds[0].name, "id");

    transpiler_manifest bundle;
    push_names(bundle, {"id"});
    const lc_expr* body = bundle.tx.transpile(ds[0].body);
    pop_names(bundle, 1);

    EXPECT_EQ(body, lc_abs(bundle.lc, lc_var(bundle.lc, 0)));
}

TEST(ParseTranspileTest, NotFunctionUsesGlobalIndices) {
    auto mod = parse_module("true/0 | false/0.\nnot = b => b false true.");
    auto ds = definitions_from(mod);
    ASSERT_EQ(ds.size(), 1u);

    transpiler_manifest bundle;
    push_names(bundle, kBuiltinNames);
    const lc_expr* body = bundle.tx.transpile(ds[0].body);
    pop_names(bundle, kBuiltinNames.size());

    const lc_expr* expected = lc_abs(bundle.lc, bundle.lc.make_app(
        bundle.lc.make_app(lc_var(bundle.lc, 0), lc_var(bundle.lc, 5)),
        lc_var(bundle.lc, 6)));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, DeclarationGroupAndNatLiteral) {
    auto ds = definitions_from(parse_module("main = 2N."));
    ASSERT_EQ(ds.size(), 1u);

    transpiler_manifest bundle;
    push_names(bundle, kBuiltinNames);
    const lc_expr* body = bundle.tx.transpile(ds[0].body);
    pop_names(bundle, kBuiltinNames.size());

    EXPECT_NE(body, nullptr);
    EXPECT_GT(bundle.lc.size(), 1u);
}

TEST(ParseTranspileTest, IfThenElseFunctionFragment) {
    auto ds = definitions_from(parse_module("if_then_else = cond => a => b => cond a b."));
    ASSERT_EQ(ds.size(), 1u);

    transpiler_manifest bundle;
    const std::vector<std::string> names = [] {
        auto v = kBuiltinNames;
        v.push_back("if_then_else");
        return v;
    }();
    push_names(bundle, names);
    const lc_expr* body = bundle.tx.transpile(ds[0].body);
    pop_names(bundle, names.size());

    const lc_expr* expected = lc_abs(bundle.lc, lc_abs(bundle.lc, lc_abs(bundle.lc,
        bundle.lc.make_app(
            bundle.lc.make_app(lc_var(bundle.lc, 2), lc_var(bundle.lc, 1)),
            lc_var(bundle.lc, 0)))));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, MainUsesGlobalIndicesNotDeltaInlining) {
    auto ds = definitions_from(parse_module(
        "if_then_else = cond => a => b => cond a b.\n"
        "main = if_then_else true false true."));
    ASSERT_EQ(ds.size(), 2u);

    transpiler_manifest bundle;
    const std::vector<std::string> names = [] {
        auto v = kBuiltinNames;
        v.push_back("if_then_else");
        v.push_back("main");
        return v;
    }();
    push_names(bundle, names);
    const lc_expr* main_body = bundle.tx.transpile(ds[1].body);
    const uint32_t k_ite   = bundle.sc.get_var_index("if_then_else");
    const uint32_t k_true  = bundle.sc.get_var_index("true");
    const uint32_t k_false = bundle.sc.get_var_index("false");
    pop_names(bundle, names.size());

    const lc_expr* expected = bundle.lc.make_app(
        bundle.lc.make_app(
            bundle.lc.make_app(lc_var(bundle.lc, k_ite), lc_var(bundle.lc, k_true)),
            lc_var(bundle.lc, k_false)),
        lc_var(bundle.lc, k_true));
    EXPECT_EQ(main_body, expected);
}

TEST(ParseTranspileTest, ListAndIntegerLiterals) {
    auto ds = definitions_from(parse_module("main = [1, -2, 'a']."));
    ASSERT_EQ(ds.size(), 1u);

    transpiler_manifest bundle;
    push_names(bundle, kBuiltinNames);
    const lc_expr* body = bundle.tx.transpile(ds[0].body);
    pop_names(bundle, kBuiltinNames.size());

    EXPECT_NE(body, nullptr);
    EXPECT_GT(bundle.lc.size(), 5u);
}

TEST(ParseTranspileTest, StatementFileDataPointFragment) {
    antlr4::ANTLRInputStream train_stream("multiply 3 4 12 : true.");
    AMLLexer train_lexer(&train_stream);
    antlr4::CommonTokenStream train_tokens(&train_lexer);
    AMLParser train_parser(&train_tokens);
    train_parser.removeErrorListeners();
    train_lexer.removeErrorListeners();

    statement_file data = make_visitor().parse_statement_file(train_parser.statementFile());
    ASSERT_EQ(data.statements.size(), 1u);

    transpiler_manifest bundle;
    const std::vector<std::string> names = [] {
        auto v = kBuiltinNames;
        v.push_back("multiply");
        return v;
    }();
    push_names(bundle, names);
    const lc_expr* input = bundle.tx.transpile(data.statements[0].input);
    pop_names(bundle, names.size());

    EXPECT_NE(input, nullptr);
}

TEST(ParseTranspileTest, ComposeIdIdFragment) {
    auto ds = definitions_from(parse_module(
        "id = x => x.\n"
        "compose = f => g => x => f (g x).\n"
        "main = compose id id."));
    ASSERT_EQ(ds.size(), 3u);

    transpiler_manifest bundle;
    const std::vector<std::string> names = [] {
        auto v = kBuiltinNames;
        v.push_back("id");
        v.push_back("compose");
        v.push_back("main");
        return v;
    }();
    push_names(bundle, names);
    const lc_expr* main_body = bundle.tx.transpile(ds[2].body);
    const uint32_t k_compose = bundle.sc.get_var_index("compose");
    const uint32_t k_id      = bundle.sc.get_var_index("id");
    pop_names(bundle, names.size());

    const lc_expr* expected = bundle.lc.make_app(
        bundle.lc.make_app(lc_var(bundle.lc, k_compose), lc_var(bundle.lc, k_id)),
        lc_var(bundle.lc, k_id));
    EXPECT_EQ(main_body, expected);
}
