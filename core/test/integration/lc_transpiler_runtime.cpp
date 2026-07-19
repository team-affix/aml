// Integration tests for lc_transpiler_runtime.
//
// Exercises the two public methods through real aml_expr / lc_expr pools.
// No access to manifest internals — assertions are on the returned lc_expr*
// values or on the final assembled term.
//
// Because each lc_transpiler_runtime owns its own internal lc_expr_pool,
// pointer equality only holds within the same runtime.  For cross-runtime
// assertions we use struct_eq(), which recursively compares by structure.

#include <stdexcept>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/lc_transpiler_runtime.hpp"
#include "value_objects/assembler_manifest.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"

namespace {

// ---- structural equality for cross-pool comparison -------------------------

static bool struct_eq(const lc_expr* a, const lc_expr* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->content.index() != b->content.index()) return false;
    if (auto* va = std::get_if<lc_expr::var>(&a->content))
        return *va == std::get<lc_expr::var>(b->content);
    if (auto* aa = std::get_if<lc_expr::abs>(&a->content))
        return struct_eq(aa->body, std::get<lc_expr::abs>(b->content).body);
    const auto& ap_a = std::get<lc_expr::app>(a->content);
    const auto& ap_b = std::get<lc_expr::app>(b->content);
    return struct_eq(ap_a.func, ap_b.func) && struct_eq(ap_a.arg, ap_b.arg);
}

// ---- helpers ---------------------------------------------------------------

static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
    declaration_group g;
    g.declarations = std::vector<declaration_decl>(decls);
    return g;
}

struct LcTranspilerRuntimeTest : public ::testing::Test {
    aml_expr_pool         aml;
    lc_transpiler_runtime rt;
    lc_expr_pool          lc; // reference pool for expected terms
};

} // namespace

// ============================================================
// transpile_definition
// ============================================================

TEST_F(LcTranspilerRuntimeTest, TranspileDefinitionSingleIdentity) {
    // f = λx.x  →  abs(var(0))
    const aml_expr* body = aml.make_abs("x", aml.make_token("x"));
    const lc_expr*  term = rt.transpile_definition({"f", body});
    ASSERT_NE(term, nullptr);
    EXPECT_TRUE(struct_eq(term, lc.make_abs(lc.make_var(0))));
}

TEST_F(LcTranspilerRuntimeTest, TranspileDefinitionBodyCannotSeeOwnName) {
    // f = λx. f  — "f" is not in scope when the body is transpiled
    const aml_expr* body = aml.make_abs("x", aml.make_token("f"));
    EXPECT_THROW(rt.transpile_definition({"f", body}), std::out_of_range);
}

TEST_F(LcTranspilerRuntimeTest, TranspileDefinitionSecondSeesFirst) {
    // f = λx.x (closed), then g = λy. f
    // After f pushed: scope depth 1; inside g's abs (depth 2): f → var(1)
    // → g's body = abs(var(1))
    const aml_expr* f_body = aml.make_abs("x", aml.make_token("x"));
    rt.transpile_definition({"f", f_body});

    const aml_expr* g_body = aml.make_abs("y", aml.make_token("f"));
    const lc_expr*  g_term = rt.transpile_definition({"g", g_body});

    ASSERT_NE(g_term, nullptr);
    EXPECT_TRUE(struct_eq(g_term, lc.make_abs(lc.make_var(1))));
}

// ============================================================
// transpile_declaration_group
// ============================================================

TEST_F(LcTranspilerRuntimeTest, TranspileDeclarationGroupScottEncoding) {
    // {true/0, false/0}
    //   true  = abs(abs(var(1)))
    //   false = abs(abs(var(0)))
    auto terms = rt.transpile_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));
    ASSERT_EQ(terms.size(), 2u);
    EXPECT_TRUE(struct_eq(terms[0], lc.make_abs(lc.make_abs(lc.make_var(1)))));
    EXPECT_TRUE(struct_eq(terms[1], lc.make_abs(lc.make_abs(lc.make_var(0)))));
}

TEST_F(LcTranspilerRuntimeTest, TranspileDeclarationGroupPushesNamesToScope) {
    // After {true/0, false/0}, transpile not = λb. b false true
    // Scope depth 2; inside not's abs (depth 3): b=var(0), false=var(1), true=var(2)
    rt.transpile_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));

    const aml_expr* not_body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));
    const lc_expr* not_term = rt.transpile_definition({"not", not_body});

    const lc_expr* expected = lc.make_abs(
        lc.make_app(
            lc.make_app(lc.make_var(0), lc.make_var(1)),
            lc.make_var(2)));
    EXPECT_TRUE(struct_eq(not_term, expected));
}

TEST_F(LcTranspilerRuntimeTest, TranspileDeclarationGroupScopeNotLeakedBeforeGroup) {
    // "true" and "false" must NOT be in scope before the group is transpiled
    const aml_expr* body = aml.make_abs("b",
        aml.make_app(aml.make_token("b"), aml.make_token("false")));
    EXPECT_THROW(rt.transpile_definition({"test", body}), std::out_of_range);
}

// ============================================================
// Full pipeline: declarations + definition + assembler
// ============================================================

TEST_F(LcTranspilerRuntimeTest, TranspileDeclarationThenDefinitionFullPipeline) {
    // Program: {true/0, false/0} + not = λb. b false true
    // Push to assembler in definition order (first-defined first).
    // LIFO pop: not innermost (var 0), false next (var 1), true outermost (var 2).
    //
    // Assembled result (outermost first):
    //   app(abs(app(abs(app(abs(nullptr), not_term)), false_term)), true_term)
    auto decl_terms = rt.transpile_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));
    ASSERT_EQ(decl_terms.size(), 2u);

    const aml_expr* not_body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));
    const lc_expr* not_term = rt.transpile_definition({"not", not_body});

    assembler_manifest asm_m;
    asm_m.globals.push(decl_terms[0]); // true  (first-defined → outermost)
    asm_m.globals.push(decl_terms[1]); // false
    asm_m.globals.push(not_term);      // not   (last-defined → innermost)

    const lc_expr* result = asm_m.asm_.assemble();
    ASSERT_NE(result, nullptr);

    // Outermost is app(abs(…), true_term)
    ASSERT_TRUE(std::holds_alternative<lc_expr::app>(result->content));
    const auto& outer = std::get<lc_expr::app>(result->content);
    EXPECT_TRUE(struct_eq(outer.arg, decl_terms[0])); // arg = true_term
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.func->content));
}
