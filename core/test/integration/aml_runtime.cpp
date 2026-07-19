// Integration tests for aml_runtime.
//
// All assertions are made via assemble() or EXPECT_THROW — no access to
// manifest internals.  Because visit_* accumulates terms internally, positive
// tests verify the assembled output structure using struct_eq().

#include <stdexcept>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/aml_runtime.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"

namespace {

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

static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
    declaration_group g;
    g.declarations = std::vector<declaration_decl>(decls);
    return g;
}

// Unwrap the arg of the outermost assembled app(abs(…), arg).
static const lc_expr* outermost_arg(const lc_expr* assembled) {
    return std::get<lc_expr::app>(assembled->content).arg;
}

// Unwrap the arg of the second assembled layer: app(abs(app(abs(…), arg)), _).
static const lc_expr* second_arg(const lc_expr* assembled) {
    const auto& outer    = std::get<lc_expr::app>(assembled->content);
    const auto& inner_abs = std::get<lc_expr::abs>(outer.func->content);
    return std::get<lc_expr::app>(inner_abs.body->content).arg;
}

struct AmlRuntimeTest : public ::testing::Test {
    aml_expr_pool aml;
    aml_runtime   rt;
    lc_expr_pool  lc; // reference pool for expected terms
};

} // namespace

// ============================================================
// visit_definition
// ============================================================

TEST_F(AmlRuntimeTest, VisitDefinitionSingleIdentity) {
    // f = λx.x  →  term = abs(var(0))
    // assemble() yields app(abs(nullptr), abs(var(0)))
    const aml_expr* body = aml.make_abs("x", aml.make_token("x"));
    rt.visit_definition({"f", body});
    const lc_expr* result = rt.assemble();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(struct_eq(outermost_arg(result), lc.make_abs(lc.make_var(0))));
}

TEST_F(AmlRuntimeTest, VisitDefinitionBodyCannotSeeOwnName) {
    // f = λx. f  — "f" is not in scope when the body is transpiled
    const aml_expr* body = aml.make_abs("x", aml.make_token("f"));
    EXPECT_THROW(rt.visit_definition({"f", body}), std::out_of_range);
}

TEST_F(AmlRuntimeTest, VisitDefinitionSecondSeesFirst) {
    // f = λx.x, then g = λy. f
    // After f pushed (depth 1), inside g's abs (depth 2): f → var(1)
    // → g's term = abs(var(1))
    //
    // assemble() — stack [f_term (bottom), g_term (top)]:
    //   LIFO pops g first (innermost), then f (outermost)
    //   result = app(abs(app(abs(nullptr), g_term)), f_term)
    //   outermost arg  = f_term = abs(var(0))
    //   second   arg   = g_term = abs(var(1))
    const aml_expr* f_body = aml.make_abs("x", aml.make_token("x"));
    const aml_expr* g_body = aml.make_abs("y", aml.make_token("f"));
    rt.visit_definition({"f", f_body});
    rt.visit_definition({"g", g_body});
    const lc_expr* result = rt.assemble();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(struct_eq(outermost_arg(result), lc.make_abs(lc.make_var(0)))); // f
    EXPECT_TRUE(struct_eq(second_arg(result),    lc.make_abs(lc.make_var(1)))); // g
}

// ============================================================
// visit_declaration_group
// ============================================================

TEST_F(AmlRuntimeTest, VisitDeclarationGroupScottEncoding) {
    // {true/0, false/0}
    //   true  = abs(abs(var(1)))  (k=0)
    //   false = abs(abs(var(0)))  (k=1)
    //
    // assemble() — stack [true_term (bottom), false_term (top)]:
    //   outermost arg = true_term, second arg = false_term
    rt.visit_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));
    const lc_expr* result = rt.assemble();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(struct_eq(outermost_arg(result), lc.make_abs(lc.make_abs(lc.make_var(1)))));
    EXPECT_TRUE(struct_eq(second_arg(result),    lc.make_abs(lc.make_abs(lc.make_var(0)))));
}

TEST_F(AmlRuntimeTest, VisitDeclarationGroupPushesNamesToScope) {
    // After {true/0, false/0} (depth 2), transpile not = λb. b false true
    // Inside not's abs (depth 3): b=var(0), false=var(1), true=var(2)
    // not's term = abs(app(app(var(0), var(1)), var(2)))
    //
    // assemble() — stack [true, false, not]:
    //   LIFO: not innermost, false middle, true outermost
    //   outermost arg = true_term
    //   second    arg = false_term
    //   third     arg = not_term = abs(app(app(var(0), var(1)), var(2)))
    rt.visit_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));

    const aml_expr* not_body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));
    rt.visit_definition({"not", not_body});
    const lc_expr* result = rt.assemble();

    // Unwrap the third (innermost) arg
    const auto& outer     = std::get<lc_expr::app>(result->content);
    const auto& mid_abs   = std::get<lc_expr::abs>(outer.func->content);
    const auto& mid_app   = std::get<lc_expr::app>(mid_abs.body->content);
    const auto& inner_abs = std::get<lc_expr::abs>(mid_app.func->content);
    const lc_expr* not_term = std::get<lc_expr::app>(inner_abs.body->content).arg;

    const lc_expr* expected = lc.make_abs(
        lc.make_app(
            lc.make_app(lc.make_var(0), lc.make_var(1)),
            lc.make_var(2)));
    EXPECT_TRUE(struct_eq(not_term, expected));
}

TEST_F(AmlRuntimeTest, VisitDeclarationGroupScopeNotLeakedBeforeGroup) {
    // "false" must NOT be in scope before the group is visited
    const aml_expr* body = aml.make_abs("b",
        aml.make_app(aml.make_token("b"), aml.make_token("false")));
    EXPECT_THROW(rt.visit_definition({"test", body}), std::out_of_range);
}

// ============================================================
// Full pipeline
// ============================================================

TEST_F(AmlRuntimeTest, FullPipelineDeclarationsAndDefinition) {
    // {true/0, false/0} + not = λb. b false true
    // assemble() yields a closed let-chain with three bindings.
    rt.visit_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));
    const aml_expr* not_body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));
    rt.visit_definition({"not", not_body});

    const lc_expr* result = rt.assemble();
    ASSERT_NE(result, nullptr);

    // Outermost is app(abs(…), true_term)
    ASSERT_TRUE(std::holds_alternative<lc_expr::app>(result->content));
    const auto& outer = std::get<lc_expr::app>(result->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.func->content));
    EXPECT_TRUE(struct_eq(outer.arg, lc.make_abs(lc.make_abs(lc.make_var(1))))); // true
}
