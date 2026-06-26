// aml_visitor: parses the three AML file kinds into value objects.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "parser_fixture.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"

namespace {

struct MockMakeAml {
    MOCK_METHOD(const aml_expr*, make_app, (const aml_expr* func, const aml_expr* arg));
    MOCK_METHOD(const aml_expr*, make_abs, (std::string param, const aml_expr* body));
    MOCK_METHOD(const aml_expr*, make_token, (std::string name));
    MOCK_METHOD(const aml_expr*, make_nat, (uint64_t value, nat_format format));
    MOCK_METHOD(const aml_expr*, make_integer, (int64_t value));
    MOCK_METHOD(const aml_expr*, make_character, (char value));
    MOCK_METHOD(const aml_expr*, make_string, (std::string value));
    MOCK_METHOD(const aml_expr*, make_list, (std::vector<const aml_expr*> elems, list_format format));
};

void wire_mock_to_pool(MockMakeAml& mock, aml_expr_pool& pool) {
    using testing::_;
    ON_CALL(mock, make_app(_, _)).WillByDefault([&](const aml_expr* f, const aml_expr* a) {
        return pool.make_app(f, a);
    });
    ON_CALL(mock, make_abs(_, _)).WillByDefault([&](std::string p, const aml_expr* b) {
        return pool.make_abs(std::move(p), b);
    });
    ON_CALL(mock, make_token(_)).WillByDefault([&](std::string n) {
        return pool.make_token(std::move(n));
    });
    ON_CALL(mock, make_nat(_, _)).WillByDefault([&](uint64_t v, nat_format f) {
        return pool.make_nat(v, f);
    });
    ON_CALL(mock, make_integer(_)).WillByDefault([&](int64_t v) {
        return pool.make_integer(v);
    });
    ON_CALL(mock, make_character(_)).WillByDefault([&](char ch) {
        return pool.make_character(ch);
    });
    ON_CALL(mock, make_string(_)).WillByDefault([&](std::string s) {
        return pool.make_string(std::move(s));
    });
    ON_CALL(mock, make_list(_, _)).WillByDefault([&](std::vector<const aml_expr*> e, list_format f) {
        return pool.make_list(std::move(e), f);
    });
}

static std::optional<declaration_file> try_parse_declaration_file(const std::string& src,
                                                                aml_expr_pool& pool) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    auto* tree = parser.declarationFile();
    if (parser.getNumberOfSyntaxErrors() != 0 || lexer.getNumberOfSyntaxErrors() != 0)
        return std::nullopt;
    MockMakeAml mock;
    wire_mock_to_pool(mock, pool);
    aml_visitor<MockMakeAml> visitor{mock};
    return visitor.parse_declaration_file(tree);
}

static std::optional<definition_file> try_parse_definition_file(const std::string& src,
                                                            aml_expr_pool& pool) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    auto* tree = parser.definitionFile();
    if (parser.getNumberOfSyntaxErrors() != 0 || lexer.getNumberOfSyntaxErrors() != 0)
        return std::nullopt;
    MockMakeAml mock;
    wire_mock_to_pool(mock, pool);
    aml_visitor<MockMakeAml> visitor{mock};
    return visitor.parse_definition_file(tree);
}

static std::optional<statement_file> try_parse_statement_file(const std::string& src,
                                                       aml_expr_pool& pool) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    auto* tree = parser.statementFile();
    if (parser.getNumberOfSyntaxErrors() != 0 || lexer.getNumberOfSyntaxErrors() != 0)
        return std::nullopt;
    MockMakeAml mock;
    wire_mock_to_pool(mock, pool);
    aml_visitor<MockMakeAml> visitor{mock};
    return visitor.parse_statement_file(tree);
}

struct parsed_declaration_file {
    aml_expr_pool    pool;
    declaration_file file;
};

struct parsed_definition_file {
    aml_expr_pool   pool;
    definition_file file;
};

struct parsed_statement_file {
    aml_expr_pool pool;
    statement_file file;
};

static parsed_declaration_file parse_declaration_file(const std::string& src) {
    aml_expr_pool pool;
    auto file = try_parse_declaration_file(src, pool);
    if (!file.has_value()) {
        ADD_FAILURE() << "failed to parse declaration file:\n" << src;
        return {};
    }
    return {std::move(pool), std::move(*file)};
}

static parsed_definition_file parse_definition_file(const std::string& src) {
    aml_expr_pool pool;
    auto file = try_parse_definition_file(src, pool);
    if (!file.has_value()) {
        ADD_FAILURE() << "failed to parse definition file:\n" << src;
        return {};
    }
    return {std::move(pool), std::move(*file)};
}

static parsed_statement_file parse_statement_file(const std::string& src) {
    aml_expr_pool pool;
    auto file = try_parse_statement_file(src, pool);
    if (!file.has_value()) {
        ADD_FAILURE() << "failed to parse statement file:\n" << src;
        return {};
    }
    return {std::move(pool), std::move(*file)};
}

static const aml_expr* probe_body(const definition_file& defs) {
    if (defs.definitions.size() != 1u) {
        ADD_FAILURE() << "expected one definition, got " << defs.definitions.size();
        return nullptr;
    }
    if (defs.definitions[0].name != "_aml_probe") {
        ADD_FAILURE() << "unexpected probe name: " << defs.definitions[0].name;
        return nullptr;
    }
    return defs.definitions[0].body;
}

struct parsed_expr {
    aml_expr_pool   pool;
    definition_file file;
    const aml_expr* body() const { return probe_body(file); }
};

static parsed_expr parse_expr(const std::string& expr_src) {
    parsed_definition_file pd = parse_definition_file("_aml_probe = " + expr_src + ".");
    return {std::move(pd.pool), std::move(pd.file)};
}

// ---------------------------------------------------------------------------
// aml_expr inspection helpers
// ---------------------------------------------------------------------------

static const aml_expr::token* as_token(const aml_expr* e) {
    return e ? std::get_if<aml_expr::token>(&e->content) : nullptr;
}

static const aml_expr::abs* as_abs(const aml_expr* e) {
    return e ? std::get_if<aml_expr::abs>(&e->content) : nullptr;
}

static const aml_expr::app* as_app(const aml_expr* e) {
    return e ? std::get_if<aml_expr::app>(&e->content) : nullptr;
}

static const aml_expr::nat* as_nat(const aml_expr* e) {
    return e ? std::get_if<aml_expr::nat>(&e->content) : nullptr;
}

static const aml_expr::integer* as_integer(const aml_expr* e) {
    return e ? std::get_if<aml_expr::integer>(&e->content) : nullptr;
}

static const aml_expr::character* as_character(const aml_expr* e) {
    return e ? std::get_if<aml_expr::character>(&e->content) : nullptr;
}

static const aml_expr::string* as_string(const aml_expr* e) {
    return e ? std::get_if<aml_expr::string>(&e->content) : nullptr;
}

static const aml_expr::list* as_list(const aml_expr* e) {
    return e ? std::get_if<aml_expr::list>(&e->content) : nullptr;
}

static bool is_token(const aml_expr* e, const std::string& name) {
    if (const auto* t = as_token(e))
        return t->name == name;
    return false;
}

} // namespace

struct AmlVisitorTest : AmlParserFixture {};

// ---------------------------------------------------------------------------
// Program-level
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, ParseEmptyDeclarationFile) {
    parsed_declaration_file pd = parse_declaration_file("");
    EXPECT_TRUE(pd.file.groups.empty());
}

TEST_F(AmlVisitorTest, ParseEmptyDefinitionFile) {
    parsed_definition_file pd = parse_definition_file("");
    EXPECT_TRUE(pd.file.definitions.empty());
}

TEST_F(AmlVisitorTest, ParseEmptyStatementFile) {
    parsed_statement_file pt = parse_statement_file("");
    EXPECT_TRUE(pt.file.statements.empty());
}

TEST_F(AmlVisitorTest, ParseStatementFileStatements) {
    parsed_statement_file pt =
        parse_statement_file("not false : true.\nmultiply 3 4 12 : true.");
    ASSERT_EQ(pt.file.statements.size(), 2u);
    EXPECT_NE(as_app(pt.file.statements[0].input), nullptr);
    EXPECT_NE(as_token(pt.file.statements[0].label), nullptr);
    EXPECT_NE(as_app(pt.file.statements[1].input), nullptr);
    EXPECT_NE(as_token(pt.file.statements[1].label), nullptr);
}

// ---------------------------------------------------------------------------
// Declaration groups
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitDeclarationSingle) {
    parsed_declaration_file pd = parse_declaration_file("nil/0.");
    ASSERT_EQ(pd.file.groups.size(), 1u);
    ASSERT_EQ(pd.file.groups[0].declarations.size(), 1u);
    EXPECT_EQ(pd.file.groups[0].declarations[0].name, "nil");
    EXPECT_EQ(pd.file.groups[0].declarations[0].arity, 0u);
}

TEST_F(AmlVisitorTest, VisitDeclarationGroupPair) {
    parsed_declaration_file pd = parse_declaration_file("true/0 | false/0.");
    ASSERT_EQ(pd.file.groups.size(), 1u);
    ASSERT_EQ(pd.file.groups[0].declarations.size(), 2u);
    EXPECT_EQ(pd.file.groups[0].declarations[0].name, "true");
    EXPECT_EQ(pd.file.groups[0].declarations[0].arity, 0u);
    EXPECT_EQ(pd.file.groups[0].declarations[1].name, "false");
    EXPECT_EQ(pd.file.groups[0].declarations[1].arity, 0u);
}

TEST_F(AmlVisitorTest, VisitDeclarationGroupThree) {
    parsed_declaration_file pd = parse_declaration_file("a/0 | b/1 | c/2.");
    ASSERT_EQ(pd.file.groups[0].declarations.size(), 3u);
    EXPECT_EQ(pd.file.groups[0].declarations[0].name, "a");
    EXPECT_EQ(pd.file.groups[0].declarations[1].arity, 1u);
    EXPECT_EQ(pd.file.groups[0].declarations[2].name, "c");
    EXPECT_EQ(pd.file.groups[0].declarations[2].arity, 2u);
}

TEST_F(AmlVisitorTest, VisitDeclarationGroupPreservesOrder) {
    parsed_declaration_file pd = parse_declaration_file("decided/1 | undecided/0.");
    EXPECT_EQ(pd.file.groups[0].declarations[0].name, "decided");
    EXPECT_EQ(pd.file.groups[0].declarations[1].name, "undecided");
}

TEST_F(AmlVisitorTest, VisitDeclarationHighArity) {
    parsed_declaration_file pd = parse_declaration_file("cons/2 | nil/0.");
    EXPECT_EQ(pd.file.groups[0].declarations[0].arity, 2u);
    EXPECT_EQ(pd.file.groups[0].declarations[1].arity, 0u);
}

TEST_F(AmlVisitorTest, VisitDeclarationUnderscoreName) {
    parsed_declaration_file pd = parse_declaration_file("_internal/0.");
    EXPECT_EQ(pd.file.groups[0].declarations[0].name, "_internal");
}

// ---------------------------------------------------------------------------
// Function definitions
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitFunctionDefName) {
    parsed_definition_file pd = parse_definition_file("foo = x => x.");
    ASSERT_EQ(pd.file.definitions.size(), 1u);
    EXPECT_EQ(pd.file.definitions[0].name, "foo");
}

TEST_F(AmlVisitorTest, VisitFunctionDefIdentityBody) {
    auto pe = parse_expr("x => x"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "x");
    EXPECT_TRUE(is_token(abs->body, "x"));
}

TEST_F(AmlVisitorTest, VisitFunctionDefCurried) {
    auto pe = parse_expr("x => y => x"); const aml_expr* body = pe.body();
    const auto* outer = as_abs(body);
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->param, "x");
    const auto* inner = as_abs(outer->body);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->param, "y");
    EXPECT_TRUE(is_token(inner->body, "x"));
}

TEST_F(AmlVisitorTest, VisitFunctionDefSelfReferenceAllowed) {
    auto pe = parse_expr("n => pred n"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    ASSERT_NE(abs, nullptr);
    const auto* app = as_app(abs->body);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_token(app->func, "pred"));
    EXPECT_TRUE(is_token(app->arg, "n"));
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitVarSimple) {
    auto pe = parse_expr("foo"); EXPECT_TRUE(is_token(pe.body(), "foo"));
}

TEST_F(AmlVisitorTest, VisitVarWithUnderscore) {
    auto pe = parse_expr("_x"); EXPECT_TRUE(is_token(pe.body(), "_x"));
}

// ---------------------------------------------------------------------------
// Abstractions
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitAbsSingle) {
    auto pe = parse_expr("f => f"); const auto* abs = as_abs(pe.body());
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "f");
    EXPECT_TRUE(is_token(abs->body, "f"));
}

TEST_F(AmlVisitorTest, VisitAbsTripleCurried) {
    auto pe = parse_expr("a => b => c => a"); const aml_expr* body = pe.body();
    const auto* a = as_abs(body);
    const auto* b = as_abs(a->body);
    const auto* c = as_abs(b->body);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(a->param, "a");
    EXPECT_EQ(b->param, "b");
    EXPECT_EQ(c->param, "c");
    EXPECT_TRUE(is_token(c->body, "a"));
}

TEST_F(AmlVisitorTest, VisitIfThenElseShape) {
    auto pe = parse_expr("cond => a => b => cond a b"); const aml_expr* body = pe.body();
    const auto* cond = as_abs(body);
    const auto* a = as_abs(cond->body);
    const auto* b = as_abs(a->body);
    const auto* app = as_app(b->body);
    ASSERT_NE(app, nullptr);
    const auto* inner_app = as_app(app->func);
    ASSERT_NE(inner_app, nullptr);
    EXPECT_TRUE(is_token(inner_app->func, "cond"));
    EXPECT_TRUE(is_token(inner_app->arg, "a"));
    EXPECT_TRUE(is_token(app->arg, "b"));
}

// ---------------------------------------------------------------------------
// Applications (left-associativity)
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitAppUnary) {
    auto pe = parse_expr("f x"); const auto* app = as_app(pe.body());
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_token(app->func, "f"));
    EXPECT_TRUE(is_token(app->arg, "x"));
}

TEST_F(AmlVisitorTest, VisitAppBinaryLeftAssoc) {
    auto pe = parse_expr("f x y"); const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(is_token(outer->arg, "y"));
    const auto* inner = as_app(outer->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_token(inner->func, "f"));
    EXPECT_TRUE(is_token(inner->arg, "x"));
}

TEST_F(AmlVisitorTest, VisitAppTernaryLeftAssoc) {
    auto pe = parse_expr("f x y z"); const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    const auto* middle = as_app(outer->func);
    const auto* inner = as_app(middle->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_token(inner->func, "f"));
    EXPECT_TRUE(is_token(inner->arg, "x"));
    EXPECT_TRUE(is_token(middle->arg, "y"));
    EXPECT_TRUE(is_token(outer->arg, "z"));
}

TEST_F(AmlVisitorTest, VisitAppGroupedFunction) {
    auto pe = parse_expr("(f x) y");
    const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(is_token(outer->arg, "y"));
    const auto* inner = as_app(outer->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_token(inner->func, "f"));
    EXPECT_TRUE(is_token(inner->arg, "x"));
}

TEST_F(AmlVisitorTest, VisitAppGroupedAbstractionFunction) {
    auto pe = parse_expr("(x => x) y");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    const auto* abs = as_abs(app->func);
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "x");
    EXPECT_TRUE(is_token(app->arg, "y"));
}

TEST_F(AmlVisitorTest, VisitAppAbstractionArg) {
    auto pe = parse_expr("f x => y");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_token(app->func, "f"));
    const auto* arg_abs = as_abs(app->arg);
    ASSERT_NE(arg_abs, nullptr);
    EXPECT_EQ(arg_abs->param, "x");
    EXPECT_TRUE(is_token(arg_abs->body, "y"));
}

TEST_F(AmlVisitorTest, VisitAppGroupedArg) {
    auto pe = parse_expr("f (g x)");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_token(app->func, "f"));
    const auto* inner = as_app(app->arg);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_token(inner->func, "g"));
    EXPECT_TRUE(is_token(inner->arg, "x"));
}

TEST_F(AmlVisitorTest, VisitAppDeeplyNestedArg) {
    auto pe = parse_expr("f (g (h x))");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    const auto* g_app = as_app(app->arg);
    const auto* h_app = as_app(g_app->arg);
    ASSERT_NE(h_app, nullptr);
    EXPECT_TRUE(is_token(h_app->func, "h"));
    EXPECT_TRUE(is_token(h_app->arg, "x"));
}

TEST_F(AmlVisitorTest, VisitAppTwoGroupedFunctions) {
    auto pe = parse_expr("(f x) (g y)");
    const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    const auto* f_app = as_app(outer->func);
    const auto* g_app = as_app(outer->arg);
    ASSERT_NE(f_app, nullptr);
    ASSERT_NE(g_app, nullptr);
    EXPECT_TRUE(is_token(f_app->func, "f"));
    EXPECT_TRUE(is_token(g_app->func, "g"));
}

TEST_F(AmlVisitorTest, VisitGroupedExprPassthrough) {
    auto pe = parse_expr("(x)");
    EXPECT_TRUE(is_token(pe.body(), "x"));
}

// ---------------------------------------------------------------------------
// Y combinator shape
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitYCombinatorShape) {
    auto pe = parse_expr("f => (x => f (x x)) (x => f (x x))");
    const aml_expr* body = pe.body();
    const auto* outer_abs = as_abs(body);
    ASSERT_NE(outer_abs, nullptr);
    EXPECT_EQ(outer_abs->param, "f");
    const auto* outer_app = as_app(outer_abs->body);
    ASSERT_NE(outer_app, nullptr);
    const auto* left_abs = as_abs(outer_app->func);
    const auto* right_abs = as_abs(outer_app->arg);
    ASSERT_NE(left_abs, nullptr);
    ASSERT_NE(right_abs, nullptr);
    EXPECT_EQ(left_abs->param, "x");
    EXPECT_EQ(right_abs->param, "x");
}

// ---------------------------------------------------------------------------
// Nat literals
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitNatZeroDefaultScott) {
    auto pe = parse_expr("0N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 0u);
    EXPECT_EQ(nat->format, nat_format::scott);
}

TEST_F(AmlVisitorTest, VisitNatValue) {
    auto pe = parse_expr("42N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 42u);
    EXPECT_EQ(nat->format, nat_format::scott);
}

TEST_F(AmlVisitorTest, VisitNatChurch) {
    auto pe = parse_expr("<church> 42N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 42u);
    EXPECT_EQ(nat->format, nat_format::church);
}

TEST_F(AmlVisitorTest, VisitNatScottExplicit) {
    auto pe = parse_expr("<scott> 7N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 7u);
    EXPECT_EQ(nat->format, nat_format::scott);
}

TEST_F(AmlVisitorTest, VisitNatLarge) {
    auto pe = parse_expr("1000N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 1000u);
}

// ---------------------------------------------------------------------------
// Integer literals
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitIntZero) {
    auto pe = parse_expr("0"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 0);
}

TEST_F(AmlVisitorTest, VisitIntPositive) {
    auto pe = parse_expr("42"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 42);
}

TEST_F(AmlVisitorTest, VisitIntNegative) {
    auto pe = parse_expr("-12"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, -12);
}

TEST_F(AmlVisitorTest, VisitIntNegativeLarge) {
    auto pe = parse_expr("-100"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, -100);
}

// ---------------------------------------------------------------------------
// Character literals
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitCharSimple) {
    auto pe = parse_expr("'a'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->value, 'a');
}

TEST_F(AmlVisitorTest, VisitCharNewline) {
    auto pe = parse_expr("'\\n'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->value, '\n');
}

TEST_F(AmlVisitorTest, VisitCharTab) {
    auto pe = parse_expr("'\\t'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->value, '\t');
}

TEST_F(AmlVisitorTest, VisitCharEscapedQuote) {
    auto pe = parse_expr("'\\''"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->value, '\'');
}

TEST_F(AmlVisitorTest, VisitCharBackslash) {
    auto pe = parse_expr("'\\\\'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->value, '\\');
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitStrEmpty) {
    auto pe = parse_expr("\"\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(str->value.empty());
}

TEST_F(AmlVisitorTest, VisitStrSimple) {
    auto pe = parse_expr("\"hello\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "hello");
}

TEST_F(AmlVisitorTest, VisitStrWithSpace) {
    auto pe = parse_expr("\"hello world\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "hello world");
}

TEST_F(AmlVisitorTest, VisitStrEscapedQuote) {
    auto pe = parse_expr("\"it\\\"s fine\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "it\"s fine");
}

// ---------------------------------------------------------------------------
// List literals
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitListEmptyDefaultScott) {
    auto pe = parse_expr("[]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_TRUE(lst->elems.empty());
    EXPECT_EQ(lst->format, list_format::scott);
}

TEST_F(AmlVisitorTest, VisitListOneElement) {
    auto pe = parse_expr("[a]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 1u);
    EXPECT_TRUE(is_token(lst->elems[0], "a"));
}

TEST_F(AmlVisitorTest, VisitListThreeElements) {
    auto pe = parse_expr("[a, b, c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 3u);
    EXPECT_TRUE(is_token(lst->elems[0], "a"));
    EXPECT_TRUE(is_token(lst->elems[1], "b"));
    EXPECT_TRUE(is_token(lst->elems[2], "c"));
}

TEST_F(AmlVisitorTest, VisitListNested) {
    auto pe = parse_expr("[[a, b], c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 2u);
    const auto* inner = as_list(lst->elems[0]);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_token(inner->elems[0], "a"));
    EXPECT_TRUE(is_token(lst->elems[1], "c"));
}

TEST_F(AmlVisitorTest, VisitListWithApplications) {
    auto pe = parse_expr("[f x, g y]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 2u);
    EXPECT_NE(as_app(lst->elems[0]), nullptr);
    EXPECT_NE(as_app(lst->elems[1]), nullptr);
}

TEST_F(AmlVisitorTest, VisitListChurch) {
    auto pe = parse_expr("<church> [a, b, c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_EQ(lst->format, list_format::church);
    EXPECT_EQ(lst->elems.size(), 3u);
}

TEST_F(AmlVisitorTest, VisitListScottExplicit) {
    auto pe = parse_expr("<scott> []"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_EQ(lst->format, list_format::scott);
}

TEST_F(AmlVisitorTest, VisitListMixedElements) {
    auto pe = parse_expr("[x, 42N, \"hi\", 'a']"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 4u);
    EXPECT_NE(as_token(lst->elems[0]), nullptr);
    EXPECT_NE(as_nat(lst->elems[1]), nullptr);
    EXPECT_NE(as_string(lst->elems[2]), nullptr);
    EXPECT_NE(as_character(lst->elems[3]), nullptr);
}

// ---------------------------------------------------------------------------
// Realistic programs
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, VisitStandardLibraryDeclarationFile) {
    parsed_declaration_file pd = parse_declaration_file(
        "true/0 | false/0.\n"
        "suc/1 | zero/0.\n");
    EXPECT_EQ(pd.file.groups.size(), 2u);
}

TEST_F(AmlVisitorTest, VisitStandardLibraryDefinitionFile) {
    parsed_definition_file pd = parse_definition_file(
        "Y = f => (x => f (x x)) (x => f (x x)).\n"
        "not = b => b false true.\n"
        "id = x => x.\n");
    ASSERT_EQ(pd.file.definitions.size(), 3u);
    EXPECT_EQ(pd.file.definitions[0].name, "Y");
    EXPECT_EQ(pd.file.definitions[1].name, "not");
    EXPECT_EQ(pd.file.definitions[2].name, "id");
    EXPECT_NE(as_abs(pd.file.definitions[2].body), nullptr);
}

TEST_F(AmlVisitorTest, VisitApplicationInFunctionBody) {
    auto pe = parse_expr("h => g h"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    const auto* app = as_app(abs->body);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_token(app->func, "g"));
    EXPECT_TRUE(is_token(app->arg, "h"));
}

TEST_F(AmlVisitorTest, VisitMultipleFunctionDefsInOrder) {
    parsed_definition_file pd = parse_definition_file("f = x => x.\ng = y => y.\nh = z => z.");
    ASSERT_EQ(pd.file.definitions.size(), 3u);
    EXPECT_EQ(pd.file.definitions[0].name, "f");
    EXPECT_EQ(pd.file.definitions[1].name, "g");
    EXPECT_EQ(pd.file.definitions[2].name, "h");
}

// ---------------------------------------------------------------------------
// Parse failures (grammar/lexer rejects — visitor never reached)
// ---------------------------------------------------------------------------

TEST_F(AmlVisitorTest, RejectNegZero) {
    aml_expr_pool pool;
    EXPECT_FALSE(try_parse_definition_file("_aml_probe = -0.", pool).has_value());
}

TEST_F(AmlVisitorTest, RejectEncodingOnInteger) {
    aml_expr_pool pool;
    EXPECT_FALSE(try_parse_definition_file("_aml_probe = <church> 42.", pool).has_value());
}

TEST_F(AmlVisitorTest, RejectEncodingOnChar) {
    aml_expr_pool pool;
    EXPECT_FALSE(try_parse_definition_file("_aml_probe = <church> 'a'.", pool).has_value());
}

TEST_F(AmlVisitorTest, RejectEncodingOnString) {
    aml_expr_pool pool;
    EXPECT_FALSE(try_parse_definition_file("_aml_probe = <church> \"hi\".", pool).has_value());
}

TEST_F(AmlVisitorTest, RejectEncodingOnName) {
    aml_expr_pool pool;
    EXPECT_FALSE(try_parse_definition_file("_aml_probe = <church> foo.", pool).has_value());
}
