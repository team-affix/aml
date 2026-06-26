// End-to-end: parse AML declaration + definition files then transpile to lc_expr.

#include <gtest/gtest.h>
#include "infrastructure/transpiler.hpp"
#include "infrastructure/aml_expr_pool.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/aml_visitor.hpp"

namespace {

static transpiled_program parse_and_transpile(const std::string& decl_src,
                                              const std::string& def_src) {
    antlr4::ANTLRInputStream decl_stream(decl_src);
    AMLLexer decl_lexer(&decl_stream);
    antlr4::CommonTokenStream decl_tokens(&decl_lexer);
    AMLParser decl_parser(&decl_tokens);
    decl_parser.removeErrorListeners();
    decl_lexer.removeErrorListeners();

    antlr4::ANTLRInputStream def_stream(def_src);
    AMLLexer def_lexer(&def_stream);
    antlr4::CommonTokenStream def_tokens(&def_lexer);
    AMLParser def_parser(&def_tokens);
    def_parser.removeErrorListeners();
    def_lexer.removeErrorListeners();

    aml_expr_pool pool;
    aml_visitor<aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool,
                aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool>
        visitor{pool, pool, pool, pool, pool, pool, pool, pool};
    declaration_file decls = visitor.parse_declarations(decl_parser.declarationFile());
    definition_file defs = visitor.parse_definitions(def_parser.definitionFile());
    return transpile_program(decls, defs);
}

const lc_expr* lc_lam(lc_expr_pool& pool, const lc_expr* b) {
    return pool.make_lam(b);
}
const lc_expr* lc_var(lc_expr_pool& pool, uint32_t i) {
    return pool.make_var(i);
}

} // namespace

TEST(ParseTranspileTest, IdentityFunction) {
    transpiled_program out = parse_and_transpile("", "id = x => x");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_EQ(out.functions[0].name, "id");
    EXPECT_TRUE(lc_expr_eq(out.functions[0].body, lc_lam(out.pool, lc_var(out.pool, 0))));
}

TEST(ParseTranspileTest, NotFunctionUsesScottBools) {
    transpiled_program out = parse_and_transpile(
        "true/0 | false/0",
        "not = b => b false true");
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
        "true/0 | false/0",
        "main = 2N");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_NE(out.globals.at("true"), nullptr);
    EXPECT_NE(out.globals.at("false"), nullptr);
    EXPECT_NE(out.functions[0].body, nullptr);
}

TEST(ParseTranspileTest, IfThenElseProgram) {
    transpiled_program out = parse_and_transpile(
        "true/0 | false/0",
        "if_then_else = cond => a => b => cond a b\n"
        "main = if_then_else true false true");
    ASSERT_EQ(out.functions.size(), 2u);
    EXPECT_TRUE(lc_expr_eq(
        out.globals.at("if_then_else"),
        out.pool.make_lam(out.pool.make_lam(out.pool.make_lam(
            out.pool.make_app(
                out.pool.make_app(out.pool.make_var(2), out.pool.make_var(1)),
                out.pool.make_var(0)))))));
}

TEST(ParseTranspileTest, ListAndIntegerLiterals) {
    transpiled_program out = parse_and_transpile("", "main = [1, -2, 'a']");
    ASSERT_EQ(out.functions.size(), 1u);
    EXPECT_GT(out.pool.size(), 5u);
}

TEST(ParseTranspileTest, StressfulProgramSnippet) {
    transpiled_program out = parse_and_transpile(
        "true/0 | false/0\n"
        "suc/1 | zero/0\n"
        "cons/2 | nil/0\n"
        "pos/1 | negsuc/1",
        "id = x => x\n"
        "compose = f => g => x => f (g x)\n"
        "main = compose id id");
    EXPECT_EQ(out.functions.size(), 3u);
    EXPECT_GE(out.globals.size(), 9u);
}
