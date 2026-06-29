// Integration tests: aml_expr (built directly) → lc_expr via transpiler_manifest.
//
// Each test constructs an aml_expr using aml_expr_pool, then feeds it through
// a transpiler_manifest (real lc_expr_pool + scope + all sub-transpilers wired
// together).  Assertions compare the resulting lc_expr* against expressions
// built with the same lc_expr_pool so that pointer equality works.
//
// These tests replace parser/test/integration/transpile.cpp.  Parser correctness
// is covered by parser/test/unit/aml_visitor.cpp; this file is solely about
// transpiler_manifest end-to-end behaviour with concrete inputs.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/transpiler_manifest.hpp"

namespace {

static const std::vector<std::string> kBuiltinNames = {
    "true", "false", "cons", "nil", "pos", "negsuc"
};

struct TranspileIntegrationTest : public ::testing::Test {
    aml_expr_pool aml;

    const lc_expr* transpile(transpiler_manifest& b, const aml_expr* e,
                              const std::vector<std::string>& names) {
        for (const auto& n : names) b.sc.push(n);
        const lc_expr* result = b.tx.transpile(e);
        for (size_t i = 0; i < names.size(); ++i) b.sc.pop();
        return result;
    }

    const lc_expr* transpile_builtins(transpiler_manifest& b, const aml_expr* e) {
        return transpile(b, e, kBuiltinNames);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Core expression forms
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, IdentityFunction) {
    transpiler_manifest b;
    // λx.x  →  abs(var(0))
    const aml_expr* e = aml.make_abs("x", aml.make_token("x"));
    EXPECT_EQ(transpile(b, e, {"id"}), b.lc.make_abs(b.lc.make_var(0)));
}

TEST_F(TranspileIntegrationTest, CurriedAbstraction) {
    transpiler_manifest b;
    // λx.λy.x  →  abs(abs(var(1)))
    const aml_expr* e = aml.make_abs("x", aml.make_abs("y", aml.make_token("x")));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_var(1))));
}

TEST_F(TranspileIntegrationTest, Application) {
    transpiler_manifest b;
    // λf.λx. f x  →  abs(abs(app(var(1), var(0))))
    const aml_expr* e = aml.make_abs("f", aml.make_abs("x",
        aml.make_app(aml.make_token("f"), aml.make_token("x"))));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(
                  b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0)))));
}

TEST_F(TranspileIntegrationTest, LocalShadowingOfGlobal) {
    transpiler_manifest b;
    // λx. x (λx.x)  →  abs(app(var(0), abs(var(0))))
    const aml_expr* e = aml.make_abs("x",
        aml.make_app(aml.make_token("x"),
                     aml.make_abs("x", aml.make_token("x"))));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(0),
                                          b.lc.make_abs(b.lc.make_var(0)))));
}

TEST_F(TranspileIntegrationTest, GlobalRefShiftsUnderNestedLambda) {
    transpiler_manifest b;
    // scope: ["a", "b"], then λx.a  →  abs(var(2))
    const aml_expr* e = aml.make_abs("x", aml.make_token("a"));
    EXPECT_EQ(transpile(b, e, {"a", "b"}),
              b.lc.make_abs(b.lc.make_var(2)));
}

TEST_F(TranspileIntegrationTest, UnboundNameThrows) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_token("missing");
    EXPECT_THROW(transpile_builtins(b, e), std::out_of_range);
}

// ---------------------------------------------------------------------------
// not = b => b false true  (checks global indices with builtins in scope)
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, NotFunctionUsesGlobalIndices) {
    transpiler_manifest b;
    // "not = b => b false true"
    // builtins pushed: true(idx5), false(idx4), cons(idx3), nil(idx2), pos(idx1), negsuc(idx0)
    // after push "b" (depth=7): b→0, negsuc→1, pos→2, nil→3, cons→4, false→5, true→6
    const aml_expr* body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));

    const lc_expr* got = transpile_builtins(b, body);
    const lc_expr* expected = b.lc.make_abs(
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(0), b.lc.make_var(5)),
            b.lc.make_var(6)));
    EXPECT_EQ(got, expected);
}

// ---------------------------------------------------------------------------
// if_then_else = cond => a => b => cond a b
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, IfThenElseFunction) {
    transpiler_manifest b;
    const aml_expr* body = aml.make_abs("cond",
        aml.make_abs("a",
            aml.make_abs("b",
                aml.make_app(
                    aml.make_app(aml.make_token("cond"), aml.make_token("a")),
                    aml.make_token("b")))));
    const lc_expr* expected = b.lc.make_abs(b.lc.make_abs(b.lc.make_abs(
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(2), b.lc.make_var(1)),
            b.lc.make_var(0)))));
    EXPECT_EQ(transpile_builtins(b, body), expected);
}

// ---------------------------------------------------------------------------
// main = compose id id  (no delta inlining — globals stay as vars)
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, ComposeIdIdNoInlining) {
    transpiler_manifest b;
    const std::vector<std::string> names = [&]{
        auto v = kBuiltinNames;
        v.push_back("compose");
        v.push_back("id");
        return v;
    }();
    const aml_expr* main = aml.make_app(
        aml.make_app(aml.make_token("compose"), aml.make_token("id")),
        aml.make_token("id"));

    const lc_expr* got = transpile(b, main, names);

    // names has 8 entries pushed in order; depth=8 after all pushes.
    // "compose" was pushed 7th (slot=7): index = 8-7 = 1
    // "id"      was pushed 8th (slot=8): index = 8-8 = 0
    EXPECT_EQ(got, b.lc.make_app(
        b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0)),
        b.lc.make_var(0)));
}

// ---------------------------------------------------------------------------
// Nat literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, NatZeroBinary) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_nat(0u, nat_format::binary);
    // nil is at index 2 in kBuiltinNames order (depth 6, get_var_index("nil")=2)
    EXPECT_EQ(transpile_builtins(b, e), b.lc.make_var(2));
}

TEST_F(TranspileIntegrationTest, NatOneBinary) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_nat(1u, nat_format::binary);
    // 1 = cons(true, nil); cons=idx3, true=idx5, nil=idx2
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.lc.make_var(3), b.lc.make_var(5)),
        b.lc.make_var(2));
    EXPECT_EQ(transpile_builtins(b, e), expected);
}

TEST_F(TranspileIntegrationTest, NatZeroChurch) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_nat(0u, nat_format::church);
    // c_0 = abs(abs(var(0)))  (closed — no scope dependency)
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_var(0))));
}

TEST_F(TranspileIntegrationTest, NatTwoChurch) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_nat(2u, nat_format::church);
    // c_2 = abs(abs(app(var(1), app(var(1), var(0)))))
    const lc_expr* expected = b.lc.make_abs(b.lc.make_abs(
        b.lc.make_app(b.lc.make_var(1),
            b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0)))));
    EXPECT_EQ(transpile_builtins(b, e), expected);
}

// ---------------------------------------------------------------------------
// Integer literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, IntegerZeroIsPos) {
    transpiler_manifest b;
    const aml_expr* e = aml.make_integer(0, nat_format::binary);
    // pos=idx1 in kBuiltinNames, nil (zero)=idx2
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_app(b.lc.make_var(1), b.lc.make_var(2)));
}

TEST_F(TranspileIntegrationTest, IntegerNegativeOneIsNegsucZero) {
    transpiler_manifest b;
    const aml_expr* e    = aml.make_integer(-1, nat_format::binary);
    const aml_expr* zero = aml.make_nat(0u, nat_format::binary);
    // negsuc=idx0, transpile_nat(0)=nil=idx2
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_app(b.lc.make_var(0), b.lc.make_var(2)));
}

// ---------------------------------------------------------------------------
// Character literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, CharEqualsNatOfAsciiCode) {
    transpiler_manifest b;
    const aml_expr* ch  = aml.make_character('A');
    const aml_expr* nat = aml.make_nat(65u, nat_format::binary);
    EXPECT_EQ(transpile_builtins(b, ch), transpile_builtins(b, nat));
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, EmptyStringIsNil) {
    transpiler_manifest b;
    EXPECT_EQ(transpile_builtins(b, aml.make_string("")), b.lc.make_var(2));
}

TEST_F(TranspileIntegrationTest, StringHiHasCorrectOrder) {
    transpiler_manifest b;
    const aml_expr* h = aml.make_character('h');
    const aml_expr* i = aml.make_character('i');
    const lc_expr*  lc_h = transpile_builtins(b, h);
    const lc_expr*  lc_i = transpile_builtins(b, i);
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.lc.make_var(3), lc_h),
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(3), lc_i),
            b.lc.make_var(2)));
    EXPECT_EQ(transpile_builtins(b, aml.make_string("hi")), expected);
}

// ---------------------------------------------------------------------------
// List literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, ScottListTwoElements) {
    transpiler_manifest b;
    const aml_expr* a  = aml.make_character('a');
    const aml_expr* bb = aml.make_character('b');
    const lc_expr*  lc_a  = transpile_builtins(b, a);
    const lc_expr*  lc_bb = transpile_builtins(b, bb);
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.lc.make_var(3), lc_a),
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(3), lc_bb),
            b.lc.make_var(2)));
    EXPECT_EQ(transpile_builtins(b, aml.make_list({a, bb}, list_format::scott)), expected);
}

TEST_F(TranspileIntegrationTest, ChurchListOneElement) {
    transpiler_manifest b;
    const aml_expr* a    = aml.make_character('a');
    const lc_expr*  lc_a = transpile_builtins(b, a);
    // [a]_church = abs(abs(app(app(var(1), a_lc), var(0))))
    const lc_expr* expected = b.lc.make_abs(b.lc.make_abs(
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(1), lc_a),
            b.lc.make_var(0))));
    EXPECT_EQ(transpile_builtins(b, aml.make_list({a}, list_format::church)), expected);
}

// ---------------------------------------------------------------------------
// Mutual defs — same scope, different bodies
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, MutualDefsFromSameScope) {
    transpiler_manifest b;
    const std::vector<std::string> names = {"f", "g"};
    // f = x => g x  →  abs(app(var(1), var(0)))  (g is at outer depth)
    // g = x => f x  →  abs(app(var(2), var(0)))  (f is one deeper than g)
    const aml_expr* f_body = aml.make_abs("x",
        aml.make_app(aml.make_token("g"), aml.make_token("x")));
    const aml_expr* g_body = aml.make_abs("x",
        aml.make_app(aml.make_token("f"), aml.make_token("x")));

    EXPECT_EQ(transpile(b, f_body, names),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0))));
    EXPECT_EQ(transpile(b, g_body, names),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(2), b.lc.make_var(0))));
}
