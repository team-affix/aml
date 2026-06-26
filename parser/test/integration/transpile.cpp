// End-to-end: parse AML source then transpile to lc_expr.

#include <gtest/gtest.h>
#include "infrastructure/transpiler.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/schema_visitor.hpp"

namespace {

static transpiled_program parse_and_transpile(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    schema_visitor visitor;
    aml_program prog = visitor.parse(parser.program());
    return transpiler::transpile_program(prog);
}

const lc_expr* lc_lam(lc_expr_pool& pool, const lc_expr* b) {
    return pool.make_lam(b);
}
const lc_expr* lc_var(lc_expr_pool& pool, uint32_t i) {
    return pool.make_var(i);
}

} // namespace

TEST(ParseTranspileTest, IdentityFunction) {
    transpiled_program out = parse_and_transpile("id = x => x");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_EQ(out.functions[0].name, "id");
    EXPECT_TRUE(lc_expr_eq(out.functions[0].body, lc_lam(out.pool, lc_var(out.pool, 0))));
}

TEST(ParseTranspileTest, NotFunctionUsesScottBools) {
    transpiled_program out = parse_and_transpile("not = b => b false true");
    ASSERT_EQ(out.functions.size(), 1u);
    const lc_expr* body = out.functions[0].body;
    const lc_expr* expected = lc_lam(out.pool,
        out.pool.make_app(
            out.pool.make_app(out.pool.make_var(0), out.globals.at("false")),
            out.globals.at("true")));
    EXPECT_TRUE(lc_expr_eq(body, expected));
}

TEST(ParseTranspileTest, ConstructorGroupAndNatLiteral) {
    transpiled_program out = parse_and_transpile(
        "true/0 | false/0\n"
        "main = 2N\n");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_NE(out.globals.at("true"), nullptr);
    EXPECT_NE(out.globals.at("false"), nullptr);
    EXPECT_NE(out.functions[0].body, nullptr);
}

TEST(ParseTranspileTest, IfThenElseProgram) {
    transpiled_program out = parse_and_transpile(
        "if_then_else = cond => a => b => cond a b\n"
        "main = if_then_else true false true\n");
    ASSERT_EQ(out.functions.size(), 2u);
    EXPECT_TRUE(lc_expr_eq(
        out.globals.at("if_then_else"),
        out.pool.make_lam(out.pool.make_lam(out.pool.make_lam(
            out.pool.make_app(
                out.pool.make_app(out.pool.make_var(2), out.pool.make_var(1)),
                out.pool.make_var(0)))))));
}

TEST(ParseTranspileTest, ListAndIntegerLiterals) {
    transpiled_program out = parse_and_transpile(
        "main = [1, -2, 'a']\n");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_GT(out.pool.size(), 5u);
}

TEST(ParseTranspileTest, StressfulProgramSnippet) {
    std::string src =
        "true/0 | false/0\n"
        "suc/1 | zero/0\n"
        "cons/2 | nil/0\n"
        "pos/1 | negsuc/1\n"
        "id = x => x\n"
        "compose = f => g => x => f (g x)\n"
        "main = compose id id\n";
    transpiled_program out = parse_and_transpile(src);
    EXPECT_EQ(out.functions.size(), 3u);
    EXPECT_GE(out.globals.size(), 9u);
}
