// Integration tests: aml_expr (built directly) → lc_expr via elaborator_manifest.
//
// Each test constructs an aml_expr using aml_expr_pool, then feeds it through
// a elaborator_manifest (real lc_expr_pool + scope + all sub-transpilers wired
// together).  Assertions compare the resulting lc_expr* against expressions
// built with the same lc_expr_pool so that pointer equality works.
//
// These tests replace parser/test/integration/transpile.cpp.  Parser correctness
// is covered by parser/test/unit/aml_visitor.cpp; this file is solely about
// elaborator_manifest end-to-end behaviour with concrete inputs.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/statement_file.hpp"

namespace {

std::vector<module_file> empty_mods;
std::vector<statement_file> empty_stmts;

struct TranspileIntegrationTest : public ::testing::Test {
    aml_expr_pool aml;
    initial_goal_exprs goals;

    const lc_expr* transpile(elaborator_manifest& b, const aml_expr* e,
                              const std::vector<std::string>& names) {
        for (const auto& n : names) b.sc.push(n);
        const lc_expr* result = b.tx.transpile(e);
        for (size_t i = 0; i < names.size(); ++i) b.sc.pop();
        return result;
    }

    // Builtins are in-place via builtin_; no scope seed needed.
    const lc_expr* transpile_builtins(elaborator_manifest& b, const aml_expr* e) {
        return b.tx.transpile(e);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Core expression forms
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, IdentityFunction) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λx.x  →  abs(var(0))
    const aml_expr* e = aml.make_abs("x", aml.make_symbol("x"));
    EXPECT_EQ(transpile(b, e, {"id"}), b.lc.make_abs(b.lc.make_var(0)));
}

TEST_F(TranspileIntegrationTest, CurriedAbstraction) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λx.λy.x  →  abs(abs(var(1)))
    const aml_expr* e = aml.make_abs("x", aml.make_abs("y", aml.make_symbol("x")));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_var(1))));
}

TEST_F(TranspileIntegrationTest, Application) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λf.λx. f x  →  abs(abs(app(var(1), var(0))))
    const aml_expr* e = aml.make_abs("f", aml.make_abs("x",
        aml.make_app(aml.make_symbol("f"), aml.make_symbol("x"))));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(
                  b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0)))));
}

TEST_F(TranspileIntegrationTest, LocalShadowingOfGlobal) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λx. x (λx.x)  →  abs(app(var(0), abs(var(0))))
    const aml_expr* e = aml.make_abs("x",
        aml.make_app(aml.make_symbol("x"),
                     aml.make_abs("x", aml.make_symbol("x"))));
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(0),
                                          b.lc.make_abs(b.lc.make_var(0)))));
}

TEST_F(TranspileIntegrationTest, GlobalRefShiftsUnderNestedLambda) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // scope: ["a", "b"], then λx.a  →  abs(var(2))
    const aml_expr* e = aml.make_abs("x", aml.make_symbol("a"));
    EXPECT_EQ(transpile(b, e, {"a", "b"}),
              b.lc.make_abs(b.lc.make_var(2)));
}

TEST_F(TranspileIntegrationTest, UnboundNameThrows) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e = aml.make_symbol("missing");
    EXPECT_THROW(transpile_builtins(b, e), std::out_of_range);
}

// ---------------------------------------------------------------------------
// not = b => b false true  (checks global indices with builtins in scope)
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, NotFunctionUsesInPlaceBuiltins) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // "not = b => b false true" — false/true are closed Scott terms
    const aml_expr* body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_symbol("b"), aml.make_symbol("false")),
            aml.make_symbol("true")));

    const lc_expr* got = transpile_builtins(b, body);
    const lc_expr* expected = b.lc.make_abs(
        b.lc.make_app(
            b.lc.make_app(b.lc.make_var(0), b.builtin_.transpile_false()),
            b.builtin_.transpile_true()));
    EXPECT_EQ(got, expected);
}

// ---------------------------------------------------------------------------
// if_then_else = cond => a => b => cond a b
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, IfThenElseFunction) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* body = aml.make_abs("cond",
        aml.make_abs("a",
            aml.make_abs("b",
                aml.make_app(
                    aml.make_app(aml.make_symbol("cond"), aml.make_symbol("a")),
                    aml.make_symbol("b")))));
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
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const std::vector<std::string> names = {"compose", "id"};
    const aml_expr* main = aml.make_app(
        aml.make_app(aml.make_symbol("compose"), aml.make_symbol("id")),
        aml.make_symbol("id"));

    const lc_expr* got = transpile(b, main, names);

    // compose + id → depth 2; compose=1, id=0.
    EXPECT_EQ(got, b.lc.make_app(
        b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0)),
        b.lc.make_var(0)));
}

// ---------------------------------------------------------------------------
// Nat literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, NatZeroBinary) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e = aml.make_nat(0u, nat_format::binary);
    EXPECT_EQ(transpile_builtins(b, e), b.builtin_.transpile_nil());
}

TEST_F(TranspileIntegrationTest, NatOneBinary) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e = aml.make_nat(1u, nat_format::binary);
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.builtin_.transpile_cons(), b.builtin_.transpile_true()),
        b.builtin_.transpile_nil());
    EXPECT_EQ(transpile_builtins(b, e), expected);
}

TEST_F(TranspileIntegrationTest, NatZeroChurch) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e = aml.make_nat(0u, nat_format::church);
    // c_0 = abs(abs(var(0)))  (closed — no scope dependency)
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_var(0))));
}

TEST_F(TranspileIntegrationTest, NatTwoChurch) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
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
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e = aml.make_integer(0, nat_format::binary);
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_app(b.builtin_.transpile_pos(), b.builtin_.transpile_nil()));
}

TEST_F(TranspileIntegrationTest, IntegerNegativeOneIsNegsucZero) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* e    = aml.make_integer(-1, nat_format::binary);
    const aml_expr* zero = aml.make_nat(0u, nat_format::binary);
    EXPECT_EQ(transpile_builtins(b, e),
              b.lc.make_app(b.builtin_.transpile_negsuc(), b.builtin_.transpile_nil()));
}

// ---------------------------------------------------------------------------
// Character literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, CharEqualsNatOfAsciiCode) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* ch  = aml.make_character('A');
    const aml_expr* nat = aml.make_nat(65u, nat_format::binary);
    EXPECT_EQ(transpile_builtins(b, ch), transpile_builtins(b, nat));
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, EmptyStringIsNil) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    EXPECT_EQ(transpile_builtins(b, aml.make_string("")), b.builtin_.transpile_nil());
}

TEST_F(TranspileIntegrationTest, StringHiHasCorrectOrder) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* h = aml.make_character('h');
    const aml_expr* i = aml.make_character('i');
    const lc_expr*  lc_h = transpile_builtins(b, h);
    const lc_expr*  lc_i = transpile_builtins(b, i);
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.builtin_.transpile_cons(), lc_h),
        b.lc.make_app(
            b.lc.make_app(b.builtin_.transpile_cons(), lc_i),
            b.builtin_.transpile_nil()));
    EXPECT_EQ(transpile_builtins(b, aml.make_string("hi")), expected);
}

// ---------------------------------------------------------------------------
// List literals
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, ScottListTwoElements) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const aml_expr* a  = aml.make_character('a');
    const aml_expr* bb = aml.make_character('b');
    const lc_expr*  lc_a  = transpile_builtins(b, a);
    const lc_expr*  lc_bb = transpile_builtins(b, bb);
    const lc_expr* expected = b.lc.make_app(
        b.lc.make_app(b.builtin_.transpile_cons(), lc_a),
        b.lc.make_app(
            b.lc.make_app(b.builtin_.transpile_cons(), lc_bb),
            b.builtin_.transpile_nil()));
    EXPECT_EQ(transpile_builtins(b, aml.make_list({a, bb}, list_format::scott)), expected);
}

TEST_F(TranspileIntegrationTest, ChurchListOneElement) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
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
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    const std::vector<std::string> names = {"f", "g"};
    // f = x => g x  →  abs(app(var(1), var(0)))  (g is at outer depth)
    // g = x => f x  →  abs(app(var(2), var(0)))  (f is one deeper than g)
    const aml_expr* f_body = aml.make_abs("x",
        aml.make_app(aml.make_symbol("g"), aml.make_symbol("x")));
    const aml_expr* g_body = aml.make_abs("x",
        aml.make_app(aml.make_symbol("f"), aml.make_symbol("x")));

    EXPECT_EQ(transpile(b, f_body, names),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(1), b.lc.make_var(0))));
    EXPECT_EQ(transpile(b, g_body, names),
              b.lc.make_abs(b.lc.make_app(b.lc.make_var(2), b.lc.make_var(0))));
}

// ---------------------------------------------------------------------------
// 3-level nested abstraction through a real elaborator_manifest
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, TriplyCurriedAbstractionOutermostVar) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λx.λy.λz.x — x is under 3 binders; de Bruijn index = 2
    const aml_expr* e = aml.make_abs("x",
        aml.make_abs("y",
            aml.make_abs("z", aml.make_symbol("x"))));
    EXPECT_EQ(transpile(b, e, {}),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_abs(b.lc.make_var(2)))));
}

TEST_F(TranspileIntegrationTest, TriplyCurriedAbstractionMiddleVar) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // λx.λy.λz.y — y is one binder below the reference; de Bruijn index = 1
    const aml_expr* e = aml.make_abs("x",
        aml.make_abs("y",
            aml.make_abs("z", aml.make_symbol("y"))));
    EXPECT_EQ(transpile(b, e, {}),
              b.lc.make_abs(b.lc.make_abs(b.lc.make_abs(b.lc.make_var(1)))));
}

// ---------------------------------------------------------------------------
// List elements that are abstractions — scope hygiene check
//
// If transpile_abs leaks a push (fails to pop), the post-element lookups
// for nil/cons in scope will shift and the test fails or throws.
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, ScottListWithAbsElement) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // [λx.x]_scott — element transpiles to abs(var(0)); cons/nil are in-place.
    const aml_expr* id   = aml.make_abs("x", aml.make_symbol("x"));
    const aml_expr* list = aml.make_list({id}, list_format::scott);

    const lc_expr* id_lc     = b.lc.make_abs(b.lc.make_var(0));
    const lc_expr* expected  = b.lc.make_app(
        b.lc.make_app(b.builtin_.transpile_cons(), id_lc),
        b.builtin_.transpile_nil());
    EXPECT_EQ(transpile_builtins(b, list), expected);
}

TEST_F(TranspileIntegrationTest, ChurchListWithAbsElement) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    // [λx.x]_church = abs(abs(app(app(var(1), abs(var(0))), var(0))))
    // var(1)=f combinator, var(0)=x accumulator (church lambda's own params).
    // The element abs(var(0)) is computed in the enclosing scope (builtins),
    // not influenced by the two church-level lambdas (those are from make_abs,
    // not real scope pushes).
    const aml_expr* id   = aml.make_abs("x", aml.make_symbol("x"));
    const aml_expr* list = aml.make_list({id}, list_format::church);

    const lc_expr* id_lc    = b.lc.make_abs(b.lc.make_var(0));
    const lc_expr* body     = b.lc.make_app(
        b.lc.make_app(b.lc.make_var(1), id_lc),
        b.lc.make_var(0));
    const lc_expr* expected = b.lc.make_abs(b.lc.make_abs(body));
    EXPECT_EQ(transpile_builtins(b, list), expected);
}

// ---------------------------------------------------------------------------
// Two elaborator_manifest instances are fully independent
// ---------------------------------------------------------------------------

TEST_F(TranspileIntegrationTest, TwoManifestInstancesAreIndependent) {
    elaborator_manifest m1{empty_mods, empty_stmts, goals};
    elaborator_manifest m2{empty_mods, empty_stmts, goals};
    m1.sc.push("x");
    // m2 has its own empty scope — "x" must not be visible in m2.
    const aml_expr* tok = aml.make_symbol("x");
    EXPECT_THROW(m2.tx.transpile(tok), std::out_of_range);
    m1.sc.pop();
}

TEST_F(TranspileIntegrationTest, BuiltinNilResolvedInPlaceWithoutScope) {
    elaborator_manifest b{empty_mods, empty_stmts, goals};
    EXPECT_FALSE(b.sc.contains(k_nil_name));
    EXPECT_EQ(transpile_builtins(b, aml.make_nat(0u, nat_format::binary)),
              b.builtin_.transpile_nil());
}
