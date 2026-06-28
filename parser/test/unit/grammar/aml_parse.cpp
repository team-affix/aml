// AML grammar: lexer/parser unit tests for the three file kinds.
// Uses the raw ANTLR lexer/parser directly — no visitor or AST construction.

#include <gtest/gtest.h>
#include <string>
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/generated/AMLBaseVisitor.h"

namespace {

static bool parses_module_file(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.moduleFile();
    return parser.getNumberOfSyntaxErrors() == 0;
}

static bool parses_statement_file(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    parser.statementFile();
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

static size_t module_declaration_group_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.moduleFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    size_t count = 0;
    for (auto* item : ctx->moduleItem())
        if (item->declarationGroup()) ++count;
    return count;
}

static size_t module_definition_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.moduleFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    size_t count = 0;
    for (auto* item : ctx->moduleItem())
        if (item->definition()) ++count;
    return count;
}

static size_t statement_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.statementFile();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->statement().size();
}

static size_t group_declaration_count(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    auto* ctx = parser.declarationGroup();
    if (parser.getNumberOfSyntaxErrors() != 0) return SIZE_MAX;
    return ctx->declaration().size();
}

} // namespace

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

TEST(AmlParseTest, LexDeclarationName) {
    EXPECT_TRUE(parses_module_file("foo/0."));
    EXPECT_TRUE(parses_module_file("bar_baz/1."));
    EXPECT_TRUE(parses_module_file("MyType/2."));
    EXPECT_TRUE(parses_module_file("_internal/0."));
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
// Module files
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseEmptyModuleFile) {
    EXPECT_TRUE(parses_module_file(""));
}

TEST(AmlParseTest, ParseDeclarationSingle) {
    EXPECT_EQ(group_declaration_count("nil/0"), 1u);
}

TEST(AmlParseTest, ParseDeclarationGroup) {
    EXPECT_EQ(group_declaration_count("true/0 | false/0"), 2u);
}

TEST(AmlParseTest, ParseDeclarationGroupThree) {
    EXPECT_EQ(group_declaration_count("a/0 | b/1 | c/2"), 3u);
}

TEST(AmlParseTest, ParseDeclarationGroupNewlines) {
    EXPECT_EQ(group_declaration_count("true/0 |\nfalse/0"), 2u);
}

TEST(AmlParseTest, ParseMultipleDeclarationGroups) {
    std::string src =
        "true/0 | false/0.\n"
        "suc/1 | zero/0.\n";
    EXPECT_EQ(module_declaration_group_count(src), 2u);
}

TEST(AmlParseTest, ParseDefinitionSimple) {
    EXPECT_EQ(module_definition_count("not = b => b false true."), 1u);
}

TEST(AmlParseTest, ParseDefinitionMultiple) {
    EXPECT_EQ(module_definition_count("f = x => x.\ng = x => x."), 2u);
}

TEST(AmlParseTest, ParseDefinitionSelfRef) {
    EXPECT_TRUE(parses_module_file("factorial = n => n (factorial (pred n)) one."));
}

TEST(AmlParseTest, ParseModuleFileMixed) {
    std::string src =
        "nil/0 | cons/2.\n"
        "append = Y (self => a => b => a b (h => t => cons h (self t b))).\n";
    EXPECT_EQ(module_declaration_group_count(src), 1u);
    EXPECT_EQ(module_definition_count(src), 1u);
}

TEST(AmlParseTest, ParseModuleFileMixedMultiple) {
    std::string src =
        "true/0 | false/0.\n"
        "or  = a => b => a true b.\n"
        "and = a => b => a b false.\n"
        "not = a => a false true.\n";
    EXPECT_EQ(module_declaration_group_count(src), 1u);
    EXPECT_EQ(module_definition_count(src), 3u);
}

TEST(AmlParseTest, RejectBareExpressionInModuleFile) {
    EXPECT_FALSE(parses_module_file("not false"));
}

TEST(AmlParseTest, RejectStatementInModuleFile) {
    EXPECT_FALSE(parses_module_file("not false : true."));
}

// ---------------------------------------------------------------------------
// Statement files
// ---------------------------------------------------------------------------

TEST(AmlParseTest, ParseEmptyStatementFile) {
    EXPECT_TRUE(parses_statement_file(""));
}

TEST(AmlParseTest, ParseStatementFileStatements) {
    EXPECT_EQ(statement_count("true : false."), 1u);
    EXPECT_EQ(statement_count("not false : true.\nmultiply 3 4 12 : true."), 2u);
}

TEST(AmlParseTest, ParseStatementFileWithLiterals) {
    EXPECT_TRUE(parses_statement_file("multiply 3 4 12 : true.\n[1, -2, 'a'] : false."));
}

TEST(AmlParseTest, RejectDeclarationInStatementFile) {
    EXPECT_FALSE(parses_statement_file("true/0 | false/0"));
}

TEST(AmlParseTest, RejectDefinitionInStatementFile) {
    EXPECT_FALSE(parses_statement_file("id = x => x."));
}

// ---------------------------------------------------------------------------
// Expressions (shared across module and statement files)
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

TEST(AmlParseTest, ParseNatLitBinary) {
    EXPECT_TRUE(parses_expr("<binary> 7N"));
}

TEST(AmlParseTest, ParseListScottExplicit) {
    EXPECT_TRUE(parses_expr("<scott> [a, b]"));
}

TEST(AmlParseTest, ParseNatScottRejected) {
    EXPECT_FALSE(parses_expr("<scott> 7N"));
}

TEST(AmlParseTest, ParseListBinaryRejected) {
    EXPECT_FALSE(parses_expr("<binary> [a, b]"));
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
    int declaration_group_count = 0;
    int definition_count        = 0;
    int statement_count         = 0;

    std::any visitDeclarationGroup(AMLParser::DeclarationGroupContext* ctx) override {
        ++declaration_group_count;
        return visitChildren(ctx);
    }
    std::any visitDefinition(AMLParser::DefinitionContext* ctx) override {
        ++definition_count;
        return visitChildren(ctx);
    }
    std::any visitStatement(AMLParser::StatementContext* ctx) override {
        ++statement_count;
        return visitChildren(ctx);
    }
};

TEST(AmlParseTest, VisitorTraversalModuleFileDeclaration) {
    antlr4::ANTLRInputStream stream("true/0 | false/0.\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.moduleFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.declaration_group_count, 1);
    EXPECT_EQ(v.definition_count, 0);
    EXPECT_EQ(v.statement_count, 0);
}

TEST(AmlParseTest, VisitorTraversalModuleFileDefinition) {
    antlr4::ANTLRInputStream stream("f = x => x.\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.moduleFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.declaration_group_count, 0);
    EXPECT_EQ(v.definition_count, 1);
    EXPECT_EQ(v.statement_count, 0);
}

TEST(AmlParseTest, VisitorTraversalModuleFileMixed) {
    antlr4::ANTLRInputStream stream("nil/0 | cons/2.\nappend = a => b => a.\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.moduleFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.declaration_group_count, 1);
    EXPECT_EQ(v.definition_count, 1);
    EXPECT_EQ(v.statement_count, 0);
}

TEST(AmlParseTest, VisitorTraversalStatementFile) {
    antlr4::ANTLRInputStream stream("not false : true.\n");
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    auto* tree = parser.statementFile();
    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.declaration_group_count, 0);
    EXPECT_EQ(v.definition_count, 0);
    EXPECT_EQ(v.statement_count, 1);
}
