// AML grammar: lexer/parser unit tests.
// Covers constructor groups, function definitions, expressions, and encoding annotations.
// Uses the raw ANTLR lexer/parser directly — no visitor or AST construction.

#include <gtest/gtest.h>
#include <string>
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/generated/AMLBaseVisitor.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool parses_program(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.program();
    return parser.getNumberOfSyntaxErrors() == 0;
}

static bool parses_expr(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0 && lexer.getNumberOfSyntaxErrors() == 0;
}

static size_t constructor_group_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.program();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    size_t count = 0;
    for (auto* decl : ctx->declaration())
        if (decl->constructorGroup()) ++count;
    return count;
}

static size_t function_def_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.program();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    size_t count = 0;
    for (auto* decl : ctx->declaration())
        if (decl->functionDef()) ++count;
    return count;
}

static size_t group_constructor_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.constructorGroup();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->constructor().size();
}

// ---------------------------------------------------------------------------
// Lexer: constructor tokens
// ---------------------------------------------------------------------------

TEST(AmlParseTest, LexConstructorName) {
    EXPECT_TRUE(parses_program("foo/0"));
    EXPECT_TRUE(parses_program("bar_baz/1"));
    EXPECT_TRUE(parses_program("MyType/2"));
    EXPECT_TRUE(parses_program("_internal/0"));
}

TEST(AmlParseTest, LexNatLit) {
    EXPECT_TRUE(parses_expr("0N"));
    EXPECT_TRUE(parses_expr("1N"));
    EXPECT_TRUE(parses_expr("42N"));
    EXPECT_TRUE(parses_expr("1000N"));
}

TEST(AmlParseTest, LexPosInt) {
    EXPECT_TRUE(parses_expr("0"));
    EXPECT_TRUE(parses_expr("1"));
    EXPECT_TRUE(parses_expr("42"));
    EXPECT_TRUE(parses_expr("999"));
}

TEST(AmlParseTest, LexNegInt) {
    EXPECT_TRUE(parses_expr("-1"));
    EXPECT_TRUE(parses_expr("-12"));
    EXPECT_TRUE(parses_expr("-100"));
}

TEST(AmlParseTest, LexNegZeroFails) {
    // -0 has no matching lexer token and must fail
    EXPECT_FALSE(parses_expr("-0"));
}

TEST(AmlParseTest, LexCharLit) {
    EXPECT_TRUE(parses_expr("'a'"));
    EXPECT_TRUE(parses_expr("'z'"));
    EXPECT_TRUE(parses_expr("'\\n'"));
    EXPECT_TRUE(parses_expr("'\\t'"));
    EXPECT_TRUE(parses_expr("'\\''"));
}

TEST(AmlParseTest, LexStrLit) {
    EXPECT_TRUE(parses_expr("\"\""));
    EXPECT_TRUE(parses_expr("\"hello\""));
    EXPECT_TRUE(parses_expr("\"hello world\""));
    EXPECT_TRUE(parses_expr("\"it\\\"s fine\""));
}

// ---------------------------------------------------------------------------
// Constructor groups
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseConstructorSingle) {
    EXPECT_EQ(group_constructor_count("nil/0"), 1u);
    EXPECT_EQ(group_constructor_count("suc/1"), 1u);
}

TEST(AmlParseTest, ParseConstructorGroup) {
    EXPECT_EQ(group_constructor_count("true/0 | false/0"), 2u);
    EXPECT_EQ(group_constructor_count("suc/1 | zero/0"), 2u);
    EXPECT_EQ(group_constructor_count("decided/1 | undecided/0"), 2u);
}

TEST(AmlParseTest, ParseConstructorGroupThree) {
    EXPECT_EQ(group_constructor_count("a/0 | b/1 | c/2"), 3u);
}

TEST(AmlParseTest, ParseConstructorGroupNewlines) {
    // | can appear before or after newlines (whitespace is skipped)
    EXPECT_EQ(group_constructor_count("true/0 |\nfalse/0"), 2u);
    EXPECT_EQ(group_constructor_count("true/0\n| false/0"), 2u);
}

TEST(AmlParseTest, ParseConstructorGroupHighArity) {
    EXPECT_TRUE(parses_program("cons/2 | nil/0"));
}

// ---------------------------------------------------------------------------
// Function definitions
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseFunctionDefSimple) {
    EXPECT_EQ(function_def_count("Y = f => (x => f (x x)) (x => f (x x))"), 1u);
    EXPECT_EQ(function_def_count("not = b => b false true"), 1u);
}

TEST(AmlParseTest, ParseFunctionDefMultiple) {
    EXPECT_EQ(function_def_count("f = x => x\ng = x => x"), 2u);
}

TEST(AmlParseTest, ParseFunctionDefSelfRef) {
    EXPECT_TRUE(parses_program("factorial = n => n (factorial (pred n)) one"));
}

// ---------------------------------------------------------------------------
// Mixed declarations
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseMixedDeclarations) {
    std::string src =
        "true/0 | false/0\n"
        "suc/1  | zero/0\n"
        "Y = f => (x => f (x x)) (x => f (x x))\n"
        "not = b => b false true\n";
    EXPECT_EQ(constructor_group_count(src), 2u);
    EXPECT_EQ(function_def_count(src), 2u);
}

TEST(AmlParseTest, ParseEmptyProgram) {
    EXPECT_TRUE(parses_program(""));
}

// ---------------------------------------------------------------------------
// Expressions: abstraction and application
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseAbstractionSimple) {
    EXPECT_TRUE(parses_expr("x => x"));
    EXPECT_TRUE(parses_expr("x => y => x"));
    EXPECT_TRUE(parses_expr("f => x => f x"));
}

TEST(AmlParseTest, ParseApplicationSimple) {
    EXPECT_TRUE(parses_expr("f x"));
    EXPECT_TRUE(parses_expr("f x y"));
    EXPECT_TRUE(parses_expr("f x y z"));
    EXPECT_TRUE(parses_expr("f (g x)"));
}

TEST(AmlParseTest, ParseApplicationGroupedFunction) {
    EXPECT_TRUE(parses_expr("(f x) y"));
    EXPECT_TRUE(parses_expr("(f x) y z"));
    EXPECT_TRUE(parses_expr("(x => x) y"));
}

TEST(AmlParseTest, ParseApplicationAbstractionArg) {
    // Bare abstraction in argument position (no parens needed)
    EXPECT_TRUE(parses_expr("f x => y"));
    EXPECT_TRUE(parses_expr("f x => y z"));
}

TEST(AmlParseTest, ParseYCombinator) {
    EXPECT_TRUE(parses_expr("f => (x => f (x x)) (x => f (x x))"));
}

TEST(AmlParseTest, ParseIfThenElse) {
    EXPECT_TRUE(parses_expr("cond => a => b => cond a b"));
}

TEST(AmlParseTest, ParseNestedApplication) {
    EXPECT_TRUE(parses_expr("f (g (h x))"));
    EXPECT_TRUE(parses_expr("(f x) (g y)"));
}

// ---------------------------------------------------------------------------
// Expressions: literals
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseNatLitDefault) {
    EXPECT_TRUE(parses_expr("42N"));
    EXPECT_TRUE(parses_expr("0N"));
}

TEST(AmlParseTest, ParseNatLitChurch) {
    EXPECT_TRUE(parses_expr("<church> 42N"));
    EXPECT_TRUE(parses_expr("<church> 0N"));
}

TEST(AmlParseTest, ParseNatLitScott) {
    EXPECT_TRUE(parses_expr("<scott> 7N"));
}

TEST(AmlParseTest, ParseEncodingOnlyNatOrList) {
    // Encoding annotation is not valid on plain ints, chars, strings, or names
    EXPECT_FALSE(parses_expr("<church> 42"));
    EXPECT_FALSE(parses_expr("<church> 'a'"));
    EXPECT_FALSE(parses_expr("<church> \"hi\""));
    EXPECT_FALSE(parses_expr("<church> foo"));
}

TEST(AmlParseTest, ParseListEmpty) {
    EXPECT_TRUE(parses_expr("[]"));
}

TEST(AmlParseTest, ParseListElements) {
    EXPECT_TRUE(parses_expr("[a]"));
    EXPECT_TRUE(parses_expr("[a, b]"));
    EXPECT_TRUE(parses_expr("[a, b, c]"));
}

TEST(AmlParseTest, ParseListNested) {
    EXPECT_TRUE(parses_expr("[[a, b], c]"));
    EXPECT_TRUE(parses_expr("[f x, g y]"));
}

TEST(AmlParseTest, ParseListChurch) {
    EXPECT_TRUE(parses_expr("<church> [a, b, c]"));
}

TEST(AmlParseTest, ParseListScott) {
    EXPECT_TRUE(parses_expr("<scott> []"));
}

TEST(AmlParseTest, ParseCharLit) {
    EXPECT_TRUE(parses_expr("'a'"));
    EXPECT_TRUE(parses_expr("'\\n'"));
}

TEST(AmlParseTest, ParseStrLit) {
    EXPECT_TRUE(parses_expr("\"hello\""));
    EXPECT_TRUE(parses_expr("\"\""));
}

// ---------------------------------------------------------------------------
// Visitor traversal
// ---------------------------------------------------------------------------

struct TestVisitor : public AMLBaseVisitor {
    int constructor_group_count = 0;
    int function_def_count      = 0;
    int expr_count              = 0;

    std::any visitConstructorGroup(AMLParser::ConstructorGroupContext* ctx) override {
        ++constructor_group_count;
        return visitChildren(ctx);
    }
    std::any visitFunctionDef(AMLParser::FunctionDefContext* ctx) override {
        ++function_def_count;
        return visitChildren(ctx);
    }
    std::any visitExpr(AMLParser::ExprContext* ctx) override {
        ++expr_count;
        return visitChildren(ctx);
    }
};

TEST(AmlParseTest, VisitorTraversal) {
    std::string src =
        "true/0 | false/0\n"
        "f = x => x\n";

    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.program();

    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.constructor_group_count, 1);
    EXPECT_EQ(v.function_def_count, 1);
    EXPECT_GT(v.expr_count, 0);
}
