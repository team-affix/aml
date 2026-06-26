// End-to-end: parse AML fragments then transpile to lc_expr.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/builtin_constructors.hpp"
#include "infrastructure/expr_transpiler.hpp"
#include "infrastructure/scott_encoder.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/aml_visitor.hpp"

namespace {

using visitor_t = aml_visitor<aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool,
                              aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool>;

aml_expr_pool& shared_pool() {
    static aml_expr_pool pool;
    return pool;
}

visitor_t make_visitor() {
    aml_expr_pool& pool = shared_pool();
    return visitor_t{pool, pool, pool, pool, pool, pool, pool, pool};
}

global_env global_env_with_builtins_and(std::vector<std::string> extra) {
    std::vector<std::string> names;
    names.reserve(builtin_constructor_names.size() + extra.size());
    for (std::string_view name : builtin_constructor_names)
        names.emplace_back(name);
    for (std::string& name : extra)
        names.push_back(std::move(name));
    return global_env(std::move(names));
}

const lc_expr* lc_lam(lc_expr_pool& pool, const lc_expr* b) {
    return pool.make_lam(b);
}
const lc_expr* lc_var(lc_expr_pool& pool, uint32_t i) {
    return pool.make_var(i);
}

} // namespace

TEST(ParseTranspileTest, IdentityFunction) {
    antlr4::ANTLRInputStream stream("id = x => x");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 1u);
    EXPECT_EQ(defs.functions[0].name, "id");

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env({{"id"}});
    local_binding_env local;
    const lc_expr* body = tx.transpile_expr(defs.functions[0].body, local, env);
    EXPECT_EQ(body, lc_lam(lc, lc_var(lc, 0)));
}

TEST(ParseTranspileTest, NotFunctionUsesGlobalIndices) {
    antlr4::ANTLRInputStream decl_stream("true/0 | false/0");
    AMLLexer decl_lexer(&decl_stream);
    antlr4::CommonTokenStream decl_tokens(&decl_lexer);
    AMLParser decl_parser(&decl_tokens);
    decl_parser.removeErrorListeners();
    decl_lexer.removeErrorListeners();

    antlr4::ANTLRInputStream def_stream("not = b => b false true");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    visitor_t visitor = make_visitor();
    declaration_file decls = visitor.parse_declarations(decl_parser.declarationFile());
    definition_file defs = visitor.parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 1u);
    ASSERT_EQ(decls.groups.size(), 1u);

    lc_expr_pool lc;
    scott_encoder enc{lc, lc, lc};
    auto bools = enc.encode_constructor_group(decls.groups[0]);
    EXPECT_EQ(bools.size(), 2u);

    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body = tx.transpile_expr(defs.functions[0].body, local, env);
    const lc_expr* expected = lc_lam(lc, lc.make_app(
        lc.make_app(lc_var(lc, 0), lc_var(lc, 5)), lc_var(lc, 6)));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, ConstructorGroupAndNatLiteral) {
    antlr4::ANTLRInputStream def_stream("main = 2N");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 1u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body = tx.transpile_expr(defs.functions[0].body, local, env);
    EXPECT_NE(body, nullptr);
    EXPECT_GT(lc.size(), 1u);
}

TEST(ParseTranspileTest, IfThenElseFunctionFragment) {
    antlr4::ANTLRInputStream def_stream(
        "if_then_else = cond => a => b => cond a b");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 1u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_with_builtins_and({"if_then_else"});
    local_binding_env local;
    const lc_expr* body = tx.transpile_expr(defs.functions[0].body, local, env);
    const lc_expr* expected = lc_lam(lc, lc_lam(lc, lc_lam(lc,
        lc.make_app(
            lc.make_app(lc_var(lc, 2), lc_var(lc, 1)), lc_var(lc, 0)))));
    EXPECT_EQ(body, expected);
}

TEST(ParseTranspileTest, MainUsesGlobalIndicesNotDeltaInlining) {
    antlr4::ANTLRInputStream def_stream(
        "if_then_else = cond => a => b => cond a b\n"
        "main = if_then_else true false true");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 2u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_with_builtins_and({"if_then_else", "main"});
    local_binding_env local;
    const lc_expr* main_body = tx.transpile_expr(defs.functions[1].body, local, env);
    auto k_ite = env.lookup_global("if_then_else");
    auto k_true = env.lookup_global("true");
    auto k_false = env.lookup_global("false");
    ASSERT_TRUE(k_ite.has_value());
    ASSERT_TRUE(k_true.has_value());
    ASSERT_TRUE(k_false.has_value());
    const lc_expr* expected = lc.make_app(
        lc.make_app(
            lc.make_app(lc_var(lc, *k_ite), lc_var(lc, *k_true)),
            lc_var(lc, *k_false)),
        lc_var(lc, *k_true));
    EXPECT_EQ(main_body, expected);
}

TEST(ParseTranspileTest, ListAndIntegerLiterals) {
    antlr4::ANTLRInputStream def_stream("main = [1, -2, 'a']");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 1u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_from_builtin_names();
    local_binding_env local;
    const lc_expr* body = tx.transpile_expr(defs.functions[0].body, local, env);
    EXPECT_NE(body, nullptr);
    EXPECT_GT(lc.size(), 5u);
}

TEST(ParseTranspileTest, TrainingDataPointFragment) {
    antlr4::ANTLRInputStream train_stream("multiply 3 4 12.");
    AMLLexer train_lexer(&train_stream);
    antlr4::CommonTokenStream train_tokens(&train_lexer);
    AMLParser train_parser(&train_tokens);
    train_parser.removeErrorListeners();
    train_lexer.removeErrorListeners();

    training_file data = make_visitor().parse_training(train_parser.trainingFile());
    ASSERT_EQ(data.statements.size(), 1u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_with_builtins_and({"multiply"});
    local_binding_env local;
    const lc_expr* stmt = tx.transpile_expr(data.statements[0], local, env);
    EXPECT_NE(stmt, nullptr);
}

TEST(ParseTranspileTest, ComposeIdIdFragment) {
    antlr4::ANTLRInputStream def_stream(
        "id = x => x\n"
        "compose = f => g => x => f (g x)\n"
        "main = compose id id");
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    definition_file defs = make_visitor().parse_definitions(def_parser.definitionFile());
    ASSERT_EQ(defs.functions.size(), 3u);

    lc_expr_pool lc;
    expr_transpiler tx{lc, lc, lc};
    global_env env = global_env_with_builtins_and({"id", "compose", "main"});
    local_binding_env local;
    const lc_expr* main_body = tx.transpile_expr(defs.functions[2].body, local, env);
    auto k_compose = env.lookup_global("compose");
    auto k_id = env.lookup_global("id");
    ASSERT_TRUE(k_compose.has_value());
    ASSERT_TRUE(k_id.has_value());
    const lc_expr* expected = lc.make_app(
        lc.make_app(lc_var(lc, *k_compose), lc_var(lc, *k_id)), lc_var(lc, *k_id));
    EXPECT_EQ(main_body, expected);
}
