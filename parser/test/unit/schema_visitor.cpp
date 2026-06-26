// schema_visitor: parses AML source into aml_program / aml_expr via schema_visitor.
// Covers constructor groups, function definitions, expression shapes, and literals.

#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <variant>
#include "parser_fixture.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"

namespace {

// ---------------------------------------------------------------------------
// Parse helpers
// ---------------------------------------------------------------------------

static std::optional<aml_program> try_parse_program(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    auto* tree = parser.program();
    if (parser.getNumberOfSyntaxErrors() != 0 || lexer.getNumberOfSyntaxErrors() != 0)
        return std::nullopt;
    schema_visitor visitor;
    return visitor.parse(tree);
}

static aml_program parse_program(const std::string& src) {
    auto prog = try_parse_program(src);
    if (!prog.has_value()) {
        ADD_FAILURE() << "failed to parse program:\n" << src;
        return {};
    }
    return std::move(*prog);
}

static aml_program parse_expr_prog(const std::string& expr_src) {
    return parse_program("_aml_probe = " + expr_src);
}

static const aml_expr* expr_body(const aml_program& prog) {
    if (prog.functions.size() != 1u) {
        ADD_FAILURE() << "expected one function definition, got " << prog.functions.size();
        return nullptr;
    }
    if (prog.functions[0].name != "_aml_probe") {
        ADD_FAILURE() << "unexpected probe name: " << prog.functions[0].name;
        return nullptr;
    }
    return prog.functions[0].body;
}

struct parsed_expr {
    aml_program     prog;
    const aml_expr* body() const { return expr_body(prog); }
};

static parsed_expr parse_expr(const std::string& expr_src) {
    return {parse_expr_prog(expr_src)};
}

// ---------------------------------------------------------------------------
// aml_expr inspection helpers
// ---------------------------------------------------------------------------

static const aml_expr::var* as_var(const aml_expr* e) {
    return e ? std::get_if<aml_expr::var>(&e->content) : nullptr;
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

static bool is_var(const aml_expr* e, const std::string& name) {
    if (const auto* v = as_var(e))
        return v->name == name;
    return false;
}

} // namespace

struct SchemaVisitorTest : AmlParserFixture {};

// ---------------------------------------------------------------------------
// Program-level
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, ParseEmptyProgram) {
    aml_program prog = parse_program("");
    EXPECT_TRUE(prog.groups.empty());
    EXPECT_TRUE(prog.functions.empty());
}

TEST_F(SchemaVisitorTest, ParseMixedProgram) {
    std::string src =
        "true/0 | false/0\n"
        "suc/1 | zero/0\n"
        "id = x => x\n"
        "not = b => b false true\n";
    aml_program prog = parse_program(src);
    ASSERT_EQ(prog.groups.size(), 2u);
    ASSERT_EQ(prog.functions.size(), 2u);
    EXPECT_EQ(prog.functions[0].name, "id");
    EXPECT_EQ(prog.functions[1].name, "not");
}

// ---------------------------------------------------------------------------
// Constructor groups
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitConstructorSingle) {
    aml_program prog = parse_program("nil/0");
    ASSERT_EQ(prog.groups.size(), 1u);
    ASSERT_EQ(prog.groups[0].size(), 1u);
    EXPECT_EQ(prog.groups[0][0].name, "nil");
    EXPECT_EQ(prog.groups[0][0].arity, 0u);
}

TEST_F(SchemaVisitorTest, VisitConstructorGroupPair) {
    aml_program prog = parse_program("true/0 | false/0");
    ASSERT_EQ(prog.groups.size(), 1u);
    ASSERT_EQ(prog.groups[0].size(), 2u);
    EXPECT_EQ(prog.groups[0][0].name, "true");
    EXPECT_EQ(prog.groups[0][0].arity, 0u);
    EXPECT_EQ(prog.groups[0][1].name, "false");
    EXPECT_EQ(prog.groups[0][1].arity, 0u);
}

TEST_F(SchemaVisitorTest, VisitConstructorGroupThree) {
    aml_program prog = parse_program("a/0 | b/1 | c/2");
    ASSERT_EQ(prog.groups[0].size(), 3u);
    EXPECT_EQ(prog.groups[0][0].name, "a");
    EXPECT_EQ(prog.groups[0][1].arity, 1u);
    EXPECT_EQ(prog.groups[0][2].name, "c");
    EXPECT_EQ(prog.groups[0][2].arity, 2u);
}

TEST_F(SchemaVisitorTest, VisitConstructorGroupPreservesOrder) {
    aml_program prog = parse_program("decided/1 | undecided/0");
    EXPECT_EQ(prog.groups[0][0].name, "decided");
    EXPECT_EQ(prog.groups[0][1].name, "undecided");
}

TEST_F(SchemaVisitorTest, VisitConstructorHighArity) {
    aml_program prog = parse_program("cons/2 | nil/0");
    EXPECT_EQ(prog.groups[0][0].arity, 2u);
    EXPECT_EQ(prog.groups[0][1].arity, 0u);
}

TEST_F(SchemaVisitorTest, VisitConstructorUnderscoreName) {
    aml_program prog = parse_program("_internal/0");
    EXPECT_EQ(prog.groups[0][0].name, "_internal");
}

// ---------------------------------------------------------------------------
// Function definitions
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitFunctionDefName) {
    aml_program prog = parse_program("foo = x => x");
    ASSERT_EQ(prog.functions.size(), 1u);
    EXPECT_EQ(prog.functions[0].name, "foo");
}

TEST_F(SchemaVisitorTest, VisitFunctionDefIdentityBody) {
    auto pe = parse_expr("x => x"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "x");
    EXPECT_TRUE(is_var(abs->body, "x"));
}

TEST_F(SchemaVisitorTest, VisitFunctionDefCurried) {
    auto pe = parse_expr("x => y => x"); const aml_expr* body = pe.body();
    const auto* outer = as_abs(body);
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->param, "x");
    const auto* inner = as_abs(outer->body);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->param, "y");
    EXPECT_TRUE(is_var(inner->body, "x"));
}

TEST_F(SchemaVisitorTest, VisitFunctionDefSelfReferenceAllowed) {
    auto pe = parse_expr("n => pred n"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    ASSERT_NE(abs, nullptr);
    const auto* app = as_app(abs->body);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_var(app->func, "pred"));
    EXPECT_TRUE(is_var(app->arg, "n"));
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitVarSimple) {
    auto pe = parse_expr("foo"); EXPECT_TRUE(is_var(pe.body(), "foo"));
}

TEST_F(SchemaVisitorTest, VisitVarWithUnderscore) {
    auto pe = parse_expr("_x"); EXPECT_TRUE(is_var(pe.body(), "_x"));
}

// ---------------------------------------------------------------------------
// Abstractions
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitAbsSingle) {
    auto pe = parse_expr("f => f"); const auto* abs = as_abs(pe.body());
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "f");
    EXPECT_TRUE(is_var(abs->body, "f"));
}

TEST_F(SchemaVisitorTest, VisitAbsTripleCurried) {
    auto pe = parse_expr("a => b => c => a"); const aml_expr* body = pe.body();
    const auto* a = as_abs(body);
    const auto* b = as_abs(a->body);
    const auto* c = as_abs(b->body);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(a->param, "a");
    EXPECT_EQ(b->param, "b");
    EXPECT_EQ(c->param, "c");
    EXPECT_TRUE(is_var(c->body, "a"));
}

TEST_F(SchemaVisitorTest, VisitIfThenElseShape) {
    auto pe = parse_expr("cond => a => b => cond a b"); const aml_expr* body = pe.body();
    const auto* cond = as_abs(body);
    const auto* a = as_abs(cond->body);
    const auto* b = as_abs(a->body);
    const auto* app = as_app(b->body);
    ASSERT_NE(app, nullptr);
    const auto* inner_app = as_app(app->func);
    ASSERT_NE(inner_app, nullptr);
    EXPECT_TRUE(is_var(inner_app->func, "cond"));
    EXPECT_TRUE(is_var(inner_app->arg, "a"));
    EXPECT_TRUE(is_var(app->arg, "b"));
}

// ---------------------------------------------------------------------------
// Applications (left-associativity)
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitAppUnary) {
    auto pe = parse_expr("f x"); const auto* app = as_app(pe.body());
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_var(app->func, "f"));
    EXPECT_TRUE(is_var(app->arg, "x"));
}

TEST_F(SchemaVisitorTest, VisitAppBinaryLeftAssoc) {
    auto pe = parse_expr("f x y"); const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(is_var(outer->arg, "y"));
    const auto* inner = as_app(outer->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_var(inner->func, "f"));
    EXPECT_TRUE(is_var(inner->arg, "x"));
}

TEST_F(SchemaVisitorTest, VisitAppTernaryLeftAssoc) {
    auto pe = parse_expr("f x y z"); const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    const auto* middle = as_app(outer->func);
    const auto* inner = as_app(middle->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_var(inner->func, "f"));
    EXPECT_TRUE(is_var(inner->arg, "x"));
    EXPECT_TRUE(is_var(middle->arg, "y"));
    EXPECT_TRUE(is_var(outer->arg, "z"));
}

TEST_F(SchemaVisitorTest, VisitAppGroupedFunction) {
    auto pe = parse_expr("(f x) y");
    const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(is_var(outer->arg, "y"));
    const auto* inner = as_app(outer->func);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_var(inner->func, "f"));
    EXPECT_TRUE(is_var(inner->arg, "x"));
}

TEST_F(SchemaVisitorTest, VisitAppGroupedAbstractionFunction) {
    auto pe = parse_expr("(x => x) y");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    const auto* abs = as_abs(app->func);
    ASSERT_NE(abs, nullptr);
    EXPECT_EQ(abs->param, "x");
    EXPECT_TRUE(is_var(app->arg, "y"));
}

TEST_F(SchemaVisitorTest, VisitAppAbstractionArg) {
    auto pe = parse_expr("f x => y");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_var(app->func, "f"));
    const auto* arg_abs = as_abs(app->arg);
    ASSERT_NE(arg_abs, nullptr);
    EXPECT_EQ(arg_abs->param, "x");
    EXPECT_TRUE(is_var(arg_abs->body, "y"));
}

TEST_F(SchemaVisitorTest, VisitAppGroupedArg) {
    auto pe = parse_expr("f (g x)");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_var(app->func, "f"));
    const auto* inner = as_app(app->arg);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_var(inner->func, "g"));
    EXPECT_TRUE(is_var(inner->arg, "x"));
}

TEST_F(SchemaVisitorTest, VisitAppDeeplyNestedArg) {
    auto pe = parse_expr("f (g (h x))");
    const aml_expr* e = pe.body();
    const auto* app = as_app(e);
    const auto* g_app = as_app(app->arg);
    const auto* h_app = as_app(g_app->arg);
    ASSERT_NE(h_app, nullptr);
    EXPECT_TRUE(is_var(h_app->func, "h"));
    EXPECT_TRUE(is_var(h_app->arg, "x"));
}

TEST_F(SchemaVisitorTest, VisitAppTwoGroupedFunctions) {
    auto pe = parse_expr("(f x) (g y)");
    const aml_expr* e = pe.body();
    const auto* outer = as_app(e);
    const auto* f_app = as_app(outer->func);
    const auto* g_app = as_app(outer->arg);
    ASSERT_NE(f_app, nullptr);
    ASSERT_NE(g_app, nullptr);
    EXPECT_TRUE(is_var(f_app->func, "f"));
    EXPECT_TRUE(is_var(g_app->func, "g"));
}

TEST_F(SchemaVisitorTest, VisitGroupedExprPassthrough) {
    auto pe = parse_expr("(x)");
    EXPECT_TRUE(is_var(pe.body(), "x"));
}

// ---------------------------------------------------------------------------
// Y combinator shape
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitYCombinatorShape) {
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

TEST_F(SchemaVisitorTest, VisitNatZeroDefaultScott) {
    auto pe = parse_expr("0N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 0u);
    EXPECT_FALSE(nat->church);
}

TEST_F(SchemaVisitorTest, VisitNatValue) {
    auto pe = parse_expr("42N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 42u);
    EXPECT_FALSE(nat->church);
}

TEST_F(SchemaVisitorTest, VisitNatChurch) {
    auto pe = parse_expr("<church> 42N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 42u);
    EXPECT_TRUE(nat->church);
}

TEST_F(SchemaVisitorTest, VisitNatScottExplicit) {
    auto pe = parse_expr("<scott> 7N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 7u);
    EXPECT_FALSE(nat->church);
}

TEST_F(SchemaVisitorTest, VisitNatLarge) {
    auto pe = parse_expr("1000N"); const auto* nat = as_nat(pe.body());
    ASSERT_NE(nat, nullptr);
    EXPECT_EQ(nat->value, 1000u);
}

// ---------------------------------------------------------------------------
// Integer literals
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitIntZero) {
    auto pe = parse_expr("0"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 0);
}

TEST_F(SchemaVisitorTest, VisitIntPositive) {
    auto pe = parse_expr("42"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 42);
}

TEST_F(SchemaVisitorTest, VisitIntNegative) {
    auto pe = parse_expr("-12"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, -12);
}

TEST_F(SchemaVisitorTest, VisitIntNegativeLarge) {
    auto pe = parse_expr("-100"); const auto* integer = as_integer(pe.body());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, -100);
}

// ---------------------------------------------------------------------------
// Character literals
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitCharSimple) {
    auto pe = parse_expr("'a'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->codepoint, static_cast<uint32_t>('a'));
}

TEST_F(SchemaVisitorTest, VisitCharNewline) {
    auto pe = parse_expr("'\\n'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->codepoint, static_cast<uint32_t>('\n'));
}

TEST_F(SchemaVisitorTest, VisitCharTab) {
    auto pe = parse_expr("'\\t'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->codepoint, static_cast<uint32_t>('\t'));
}

TEST_F(SchemaVisitorTest, VisitCharEscapedQuote) {
    auto pe = parse_expr("'\\''"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->codepoint, static_cast<uint32_t>('\''));
}

TEST_F(SchemaVisitorTest, VisitCharBackslash) {
    auto pe = parse_expr("'\\\\'"); const auto* ch = as_character(pe.body());
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->codepoint, static_cast<uint32_t>('\\'));
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitStrEmpty) {
    auto pe = parse_expr("\"\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(str->value.empty());
}

TEST_F(SchemaVisitorTest, VisitStrSimple) {
    auto pe = parse_expr("\"hello\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "hello");
}

TEST_F(SchemaVisitorTest, VisitStrWithSpace) {
    auto pe = parse_expr("\"hello world\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "hello world");
}

TEST_F(SchemaVisitorTest, VisitStrEscapedQuote) {
    auto pe = parse_expr("\"it\\\"s fine\""); const auto* str = as_string(pe.body());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "it\"s fine");
}

// ---------------------------------------------------------------------------
// List literals
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitListEmptyDefaultScott) {
    auto pe = parse_expr("[]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_TRUE(lst->elems.empty());
    EXPECT_FALSE(lst->church);
}

TEST_F(SchemaVisitorTest, VisitListOneElement) {
    auto pe = parse_expr("[a]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 1u);
    EXPECT_TRUE(is_var(lst->elems[0], "a"));
}

TEST_F(SchemaVisitorTest, VisitListThreeElements) {
    auto pe = parse_expr("[a, b, c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 3u);
    EXPECT_TRUE(is_var(lst->elems[0], "a"));
    EXPECT_TRUE(is_var(lst->elems[1], "b"));
    EXPECT_TRUE(is_var(lst->elems[2], "c"));
}

TEST_F(SchemaVisitorTest, VisitListNested) {
    auto pe = parse_expr("[[a, b], c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 2u);
    const auto* inner = as_list(lst->elems[0]);
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(is_var(inner->elems[0], "a"));
    EXPECT_TRUE(is_var(lst->elems[1], "c"));
}

TEST_F(SchemaVisitorTest, VisitListWithApplications) {
    auto pe = parse_expr("[f x, g y]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 2u);
    EXPECT_NE(as_app(lst->elems[0]), nullptr);
    EXPECT_NE(as_app(lst->elems[1]), nullptr);
}

TEST_F(SchemaVisitorTest, VisitListChurch) {
    auto pe = parse_expr("<church> [a, b, c]"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_TRUE(lst->church);
    EXPECT_EQ(lst->elems.size(), 3u);
}

TEST_F(SchemaVisitorTest, VisitListScottExplicit) {
    auto pe = parse_expr("<scott> []"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    EXPECT_FALSE(lst->church);
}

TEST_F(SchemaVisitorTest, VisitListMixedElements) {
    auto pe = parse_expr("[x, 42N, \"hi\", 'a']"); const auto* lst = as_list(pe.body());
    ASSERT_NE(lst, nullptr);
    ASSERT_EQ(lst->elems.size(), 4u);
    EXPECT_NE(as_var(lst->elems[0]), nullptr);
    EXPECT_NE(as_nat(lst->elems[1]), nullptr);
    EXPECT_NE(as_string(lst->elems[2]), nullptr);
    EXPECT_NE(as_character(lst->elems[3]), nullptr);
}

// ---------------------------------------------------------------------------
// Realistic programs
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, VisitStandardLibrarySnippet) {
    std::string src =
        "true/0 | false/0\n"
        "suc/1 | zero/0\n"
        "Y = f => (x => f (x x)) (x => f (x x))\n"
        "not = b => b false true\n"
        "id = x => x\n";
    aml_program prog = parse_program(src);
    EXPECT_EQ(prog.groups.size(), 2u);
    EXPECT_EQ(prog.functions.size(), 3u);
    EXPECT_EQ(prog.functions[0].name, "Y");
    EXPECT_EQ(prog.functions[1].name, "not");
    EXPECT_EQ(prog.functions[2].name, "id");
    EXPECT_NE(as_abs(prog.functions[2].body), nullptr);
}

TEST_F(SchemaVisitorTest, VisitApplicationInFunctionBody) {
    auto pe = parse_expr("h => g h"); const aml_expr* body = pe.body();
    const auto* abs = as_abs(body);
    const auto* app = as_app(abs->body);
    ASSERT_NE(app, nullptr);
    EXPECT_TRUE(is_var(app->func, "g"));
    EXPECT_TRUE(is_var(app->arg, "h"));
}

TEST_F(SchemaVisitorTest, VisitMultipleFunctionDefsInOrder) {
    aml_program prog = parse_program("f = x => x\ng = y => y\nh = z => z");
    ASSERT_EQ(prog.functions.size(), 3u);
    EXPECT_EQ(prog.functions[0].name, "f");
    EXPECT_EQ(prog.functions[1].name, "g");
    EXPECT_EQ(prog.functions[2].name, "h");
}

// ---------------------------------------------------------------------------
// Parse failures (grammar/lexer rejects — visitor never reached)
// ---------------------------------------------------------------------------

TEST_F(SchemaVisitorTest, RejectNegZero) {
    EXPECT_FALSE(try_parse_program("_aml_probe = -0").has_value());
}

TEST_F(SchemaVisitorTest, RejectEncodingOnInteger) {
    EXPECT_FALSE(try_parse_program("_aml_probe = <church> 42").has_value());
}

TEST_F(SchemaVisitorTest, RejectEncodingOnChar) {
    EXPECT_FALSE(try_parse_program("_aml_probe = <church> 'a'").has_value());
}

TEST_F(SchemaVisitorTest, RejectEncodingOnString) {
    EXPECT_FALSE(try_parse_program("_aml_probe = <church> \"hi\"").has_value());
}

TEST_F(SchemaVisitorTest, RejectEncodingOnName) {
    EXPECT_FALSE(try_parse_program("_aml_probe = <church> foo").has_value());
}
