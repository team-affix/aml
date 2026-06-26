// End-to-end: parse AML fragments then transpile to lc_expr.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/global_env_factory.hpp"
#include "infrastructure/lc_transpile_bundle.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/aml_visitor.hpp"

namespace {

aml_expr_pool& shared_pool() {
    static aml_expr_pool pool;
    return pool;
}

aml_visitor<aml_expr_pool> make_visitor() {
    return aml_visitor<aml_expr_pool>{shared_pool()};
}

const lc_expr* lc_lam(lc_expr_pool& pool, const lc_expr* b) {
    return pool.make_lam(b);
}
const lc_expr* lc_var(lc_expr_pool& pool, uint32_t i) {
    return pool.make_var(i);
}

} // namespace

TEST(ParseTranspileTest, IdentityFunction) {
    antlr4::ANTLRInputStream stream("id = x => x.");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 1u);
    EXPECT_EQ(defs.definitions[0].name, "id");

    lc_transpile_bundle bundle;
    global_env env({"id"});
    local_binding_env local;
    const lc_expr* body =
        bundle.tx.transpile(defs.definitions[0].body, local, env);
    EXPECT_EQ(body, lc_lam(bundle.lc, lc_var(bundle.lc, 0)));
}

TEST(ParseTranspileTest, NotFunctionUsesGlobalIndices) {
    antlr4::ANTLRInputStream decl_stream("true/0 | false/0.");
    AMLLexer decl_lexer(&decl_stream);
    antlr4::CommonTokenStream decl_tokens(&decl_lexer);
    AMLParser decl_parser(&decl_tokens);
    decl_parser.removeErrorListeners();
    decl_lexer.removeErrorListeners();

    antlr4::ANTLRInputStream def_stream("not = b => b false true.");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    auto visitor = make_visitor();
    declaration_file ctors = visitor.parse_declaration_file(decl_parser.declarationFile());
    definition_file defs = visitor.parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 1u);
    ASSERT_EQ(ctors.groups.size(), 1u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body =
        bundle.tx.transpile(defs.definitions[0].body, local, env);
    const lc_expr* expected = lc_lam(bundle.lc, bundle.lc.make_app(
        bundle.lc.make_app(lc_var(bundle.lc, 0), lc_var(bundle.lc, 5)), lc_var(bundle.lc, 6)));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, DeclarationGroupAndNatLiteral) {
    antlr4::ANTLRInputStream def_stream("main = 2N.");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 1u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body =
        bundle.tx.transpile(defs.definitions[0].body, local, env);
    EXPECT_NE(body, nullptr);
    EXPECT_GT(bundle.lc.size(), 1u);
}

TEST(ParseTranspileTest, IfThenElseFunctionFragment) {
    antlr4::ANTLRInputStream def_stream(
        "if_then_else = cond => a => b => cond a b.");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 1u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names_and({"if_then_else"});
    local_binding_env local;
    const lc_expr* body =
        bundle.tx.transpile(defs.definitions[0].body, local, env);
    const lc_expr* expected = lc_lam(bundle.lc, lc_lam(bundle.lc, lc_lam(bundle.lc,
        bundle.lc.make_app(
            bundle.lc.make_app(lc_var(bundle.lc, 2), lc_var(bundle.lc, 1)),
            lc_var(bundle.lc, 0)))));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, MainUsesGlobalIndicesNotDeltaInlining) {
    antlr4::ANTLRInputStream def_stream(
        "if_then_else = cond => a => b => cond a b.\n"
        "main = if_then_else true false true.");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 2u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names_and({"if_then_else", "main"});
    local_binding_env local;
    const lc_expr* main_body =
        bundle.tx.transpile(defs.definitions[1].body, local, env);
    auto k_ite = env.lookup_global("if_then_else");
    auto k_true = env.lookup_global("true");
    auto k_false = env.lookup_global("false");
    ASSERT_TRUE(k_ite.has_value());
    ASSERT_TRUE(k_true.has_value());
    ASSERT_TRUE(k_false.has_value());
    const lc_expr* expected = bundle.lc.make_app(
        bundle.lc.make_app(
            bundle.lc.make_app(lc_var(bundle.lc, *k_ite), lc_var(bundle.lc, *k_true)),
            lc_var(bundle.lc, *k_false)),
        lc_var(bundle.lc, *k_true));
    EXPECT_EQ(main_body, expected);
}

TEST(ParseTranspileTest, ListAndIntegerLiterals) {
    antlr4::ANTLRInputStream def_stream("main = [1, -2, 'a'].");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 1u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body =
        bundle.tx.transpile(defs.definitions[0].body, local, env);
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

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names_and({"multiply"});
    local_binding_env local;
    const lc_expr* input =
        bundle.tx.transpile(data.statements[0].input, local, env);
    EXPECT_NE(input, nullptr);
}

TEST(ParseTranspileTest, ComposeIdIdFragment) {
    antlr4::ANTLRInputStream def_stream(
        "id = x => x.\n"
        "compose = f => g => x => f (g x).\n"
        "main = compose id id.");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definition_file(def_parser.definitionFile());
    ASSERT_EQ(defs.definitions.size(), 3u);

    lc_transpile_bundle bundle;
    global_env env = global_env_from_builtin_names_and({"id", "compose", "main"});
    local_binding_env local;
    const lc_expr* main_body =
        bundle.tx.transpile(defs.definitions[2].body, local, env);
    auto k_compose = env.lookup_global("compose");
    auto k_id = env.lookup_global("id");
    ASSERT_TRUE(k_compose.has_value());
    ASSERT_TRUE(k_id.has_value());
    const lc_expr* expected = bundle.lc.make_app(
        bundle.lc.make_app(lc_var(bundle.lc, *k_compose), lc_var(bundle.lc, *k_id)),
        lc_var(bundle.lc, *k_id));
    EXPECT_EQ(main_body, expected);
}
