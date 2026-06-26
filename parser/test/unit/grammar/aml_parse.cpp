// AML grammar: lexer/parser unit tests for the three file kinds.
// Uses the raw ANTLR lexer/parser directly — no visitor or AST construction.

#include <gtest/gtest.h>
#include <string>
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/generated/AMLBaseVisitor.h"

namespace {

static bool parses_declaration_file(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.declarationFile();
    return parser.getNumberOfSyntaxErrors() == 0;
}

static bool parses_definition_file(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.definitionFile();
    return parser.getNumberOfSyntaxErrors() == 0;
}

static bool parses_training_file(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.trainingFile();
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

static size_t declaration_group_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.declarationFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->constructorGroup().size();
}

static size_t function_def_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.definitionFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->functionDef().size();
}

static size_t training_statement_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.trainingFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->statement().size();
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

} // namespace

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

TEST(AmlParseTest, LexConstructorName) {
    EXPECT_TRUE(parses_declaration_file("foo/0"));
    EXPECT_TRUE(parses_declaration_file("bar_baz/1"));
    EXPECT_TRUE(parses_declaration_file("MyType/2"));
    EXPECT_TRUE(parses_declaration_file("_internal/0"));
}

TEST(AmlParseTest, LexNatLit) {
    EXPECT_TRUE(parses_expr("0N"));
    EXPECT_TRUE(parses_expr("42N"));
}

TEST(AmlParseTest, LexPosInt) {
    EXPECT_TRUE(parses_expr("0"));
    EXPECT_TRUE(parses_expr("42"));
}

TEST(AmlParseTest, LexNegInt) {
    EXPECT_TRUE(parses_expr("-1"));
    EXPECT_TRUE(parses_expr("-12"));
}

TEST(AmlParseTest, LexNegZeroFails) {
    EXPECT_FALSE(parses_expr("-0"));
}

TEST(AmlParseTest, LexCharLit) {
    EXPECT_TRUE(parses_expr("'a'"));
    EXPECT_TRUE(parses_expr("'\\n'"));
}

TEST(AmlParseTest, LexStrLit) {
    EXPECT_TRUE(parses_expr("\"\""));
    EXPECT_TRUE(parses_expr("\"hello\""));
}

// ---------------------------------------------------------------------------
// Declaration files
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseEmptyDeclarationFile) {
    EXPECT_TRUE(parses_declaration_file(""));
}

TEST(AmlParseTest, ParseConstructorSingle) {
    EXPECT_EQ(group_constructor_count("nil/0"), 1u);
}

TEST(AmlParseTest, ParseConstructorGroup) {
    EXPECT_EQ(group_constructor_count("true/0 | false/0"), 2u);
}

TEST(AmlParseTest, ParseConstructorGroupThree) {
    EXPECT_EQ(group_constructor_count("a/0 | b/1 | c/2"), 3u);
}

TEST(AmlParseTest, ParseConstructorGroupNewlines) {
    EXPECT_EQ(group_constructor_count("true/0 |\nfalse/0"), 2u);
}

TEST(AmlParseTest, ParseMultipleDeclarationGroups) {
    std::string src =
        "true/0 | false/0\n"
        "suc/1  | zero/0\n";
    EXPECT_EQ(declaration_group_count(src), 2u);
}

TEST(AmlParseTest, RejectFunctionInDeclarationFile) {
    EXPECT_FALSE(parses_declaration_file("id = x => x"));
}

TEST(AmlParseTest, RejectTrainingStatementInDeclarationFile) {
    EXPECT_FALSE(parses_declaration_file("not false."));
}

// ---------------------------------------------------------------------------
// Definition files
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseEmptyDefinitionFile) {
    EXPECT_TRUE(parses_definition_file(""));
}

TEST(AmlParseTest, ParseFunctionDefSimple) {
    EXPECT_EQ(function_def_count("not = b => b false true"), 1u);
}

TEST(AmlParseTest, ParseFunctionDefMultiple) {
    EXPECT_EQ(function_def_count("f = x => x\ng = x => x"), 2u);
}

TEST(AmlParseTest, ParseFunctionDefSelfRef) {
    EXPECT_TRUE(parses_definition_file("factorial = n => n (factorial (pred n)) one"));
}

TEST(AmlParseTest, RejectDeclarationInDefinitionFile) {
    EXPECT_FALSE(parses_definition_file("true/0 | false/0"));
}

TEST(AmlParseTest, RejectBareExpressionInDefinitionFile) {
    EXPECT_FALSE(parses_definition_file("not false"));
}

// ---------------------------------------------------------------------------
// Training files
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseEmptyTrainingFile) {
    EXPECT_TRUE(parses_training_file(""));
}

TEST(AmlParseTest, ParseTrainingStatements) {
    EXPECT_EQ(training_statement_count("true."), 1u);
    EXPECT_EQ(training_statement_count("not false.\nmultiply 3 4 12."), 2u);
}

TEST(AmlParseTest, ParseTrainingWithLiterals) {
    EXPECT_TRUE(parses_training_file("multiply 3 4 12.\n[1, -2, 'a']."));
}

TEST(AmlParseTest, RejectDeclarationInTrainingFile) {
    EXPECT_FALSE(parses_training_file("true/0 | false/0"));
}

TEST(AmlParseTest, RejectDefinitionInTrainingFile) {
    EXPECT_FALSE(parses_training_file("id = x => x"));
}

// ---------------------------------------------------------------------------
// Expressions (shared across definition and training files)
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseAbstractionSimple) {
    EXPECT_TRUE(parses_expr("x => x"));
    EXPECT_TRUE(parses_expr("x => y => x"));
}

TEST(AmlParseTest, ParseApplicationSimple) {
    EXPECT_TRUE(parses_expr("f x"));
    EXPECT_TRUE(parses_expr("f x y"));
    EXPECT_TRUE(parses_expr("f (g x)"));
}

TEST(AmlParseTest, ParseApplicationGroupedFunction) {
    EXPECT_TRUE(parses_expr("(f x) y"));
    EXPECT_TRUE(parses_expr("(x => x) y"));
}

TEST(AmlParseTest, ParseApplicationAbstractionArg) {
    EXPECT_TRUE(parses_expr("f x => y"));
}

TEST(AmlParseTest, ParseYCombinator) {
    EXPECT_TRUE(parses_expr("f => (x => f (x x)) (x => f (x x))"));
}

TEST(AmlParseTest, ParseIfThenElse) {
    EXPECT_TRUE(parses_expr("cond => a => b => cond a b"));
}

TEST(AmlParseTest, ParseNestedApplication) {
    EXPECT_TRUE(parses_expr("f (g (h x))"));
}

TEST(AmlParseTest, ParseNatLitDefault) {
    EXPECT_TRUE(parses_expr("42N"));
}

TEST(AmlParseTest, ParseNatLitChurch) {
    EXPECT_TRUE(parses_expr("<church> 42N"));
}

TEST(AmlParseTest, ParseNatLitScott) {
    EXPECT_TRUE(parses_expr("<scott> 7N"));
}

TEST(AmlParseTest, ParseEncodingOnlyNatOrList) {
    EXPECT_FALSE(parses_expr("<church> 42"));
    EXPECT_FALSE(parses_expr("<church> 'a'"));
    EXPECT_FALSE(parses_expr("<church> \"hi\""));
    EXPECT_FALSE(parses_expr("<church> foo"));
}

TEST(AmlParseTest, ParseListEmpty) {
    EXPECT_TRUE(parses_expr("[]"));
}

TEST(AmlParseTest, ParseListElements) {
    EXPECT_TRUE(parses_expr("[a, b, c]"));
}

TEST(AmlParseTest, ParseListNested) {
    EXPECT_TRUE(parses_expr("[[a, b], c]"));
}

TEST(AmlParseTest, ParseListChurch) {
    EXPECT_TRUE(parses_expr("<church> [a, b, c]"));
}

TEST(AmlParseTest, ParseListScott) {
    EXPECT_TRUE(parses_expr("<scott> []"));
}

TEST(AmlParseTest, ParseCharLit) {
    EXPECT_TRUE(parses_expr("'a'"));
}

TEST(AmlParseTest, ParseStrLit) {
    EXPECT_TRUE(parses_expr("\"hello\""));
}

// ---------------------------------------------------------------------------
// Visitor traversal
// ---------------------------------------------------------------------------

struct TestVisitor : public AMLBaseVisitor {
    int constructor_group_count = 0;
    int function_def_count      = 0;
    int statement_count         = 0;

    std::any visitConstructorGroup(AMLParser::ConstructorGroupContext* ctx) override {
        ++constructor_group_count;
        return visitChildren(ctx);
    }
    std::any visitFunctionDef(AMLParser::FunctionDefContext* ctx) override {
        ++function_def_count;
        return visitChildren(ctx);
    }
    std::any visitStatement(AMLParser::StatementContext* ctx) override {
        ++statement_count;
        return visitChildren(ctx);
    }
};

TEST(AmlParseTest, VisitorTraversalDeclarationFile) {
    antlr4::ANTLRInputStream stream("true/0 | false/0\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.declarationFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.constructor_group_count, 1);
    EXPECT_EQ(v.function_def_count, 0);
    EXPECT_EQ(v.statement_count, 0);
}

TEST(AmlParseTest, VisitorTraversalDefinitionFile) {
    antlr4::ANTLRInputStream stream("f = x => x\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.definitionFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.constructor_group_count, 0);
    EXPECT_EQ(v.function_def_count, 1);
    EXPECT_EQ(v.statement_count, 0);
}

TEST(AmlParseTest, VisitorTraversalTrainingFile) {
    antlr4::ANTLRInputStream stream("not false.\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.trainingFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.constructor_group_count, 0);
    EXPECT_EQ(v.function_def_count, 0);
    EXPECT_EQ(v.statement_count, 1);
}
