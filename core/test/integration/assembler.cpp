// Integration tests: assembler + lc_expr_pool.
//
// These tests verify the structural correctness of assemble() using real
// lc_expr_pool nodes, and confirm that the assembler's definition-order
// invariant matches the scope's de Bruijn assignment.

#include <gtest/gtest.h>
#include <stack>
#include <variant>
#include "infrastructure/assembler.hpp"
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/aml_runtime.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"

namespace {

using GlobalStack = std::stack<const lc_expr*>;

static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

} // namespace

// ---------------------------------------------------------------------------
// Structural correctness (lc_expr_pool only, no transpiler or scope)
// ---------------------------------------------------------------------------

TEST(AssemblerIntegrationTest, EmptyGlobalsReturnsNullptr) {
    lc_expr_pool lc;
    GlobalStack globals;
    assembler<lc_expr_pool, lc_expr_pool, GlobalStack, GlobalStack> asm_{lc, lc, globals, globals};
    EXPECT_EQ(asm_.assemble(), nullptr);
}

TEST(AssemblerIntegrationTest, TwoGlobalsFirstDefinedIsOutermost) {
    lc_expr_pool lc;
    GlobalStack globals;
    assembler<lc_expr_pool, lc_expr_pool, GlobalStack, GlobalStack> asm_{lc, lc, globals, globals};

    const lc_expr* g0 = lc.make_var(0);
    const lc_expr* g1 = lc.make_var(1);
    globals.push(g0);
    globals.push(g1);

    // g0 pushed first (bottom) → outermost.  g1 pushed last (top) → innermost.
    const lc_expr* expected =
        lc.make_app(lc.make_abs(lc.make_app(lc.make_abs(nullptr), g1)), g0);
    EXPECT_EQ(asm_.assemble(), expected);
}

// ---------------------------------------------------------------------------
// DefinitionOrderMatchesScopeOrder
//
// Verifies the paired invariant: push order = stack order = outermost-first.
//
// After pushing "true" (depth 1) then "false" (depth 2):
//   get_var_index("true")  = 2 - 1 = 1  →  var(1) in any body
//   get_var_index("false") = 2 - 2 = 0  →  var(0) in any body
//
// Pushing true_term then false_term onto globals (in definition order) and
// calling assemble() must place true_term outermost so that var(1) inside
// the assembled expression correctly refers to true_term.
// ---------------------------------------------------------------------------

TEST(AssemblerIntegrationTest, DefinitionOrderMatchesScopeOrder) {
    lc_expr_pool lc;
    GlobalStack globals;
    assembler<lc_expr_pool, lc_expr_pool, GlobalStack, GlobalStack> asm_{lc, lc, globals, globals};

    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
    auto terms = dt.transpile_group(make_group({{"true", 0u}, {"false", 0u}}));
    const lc_expr* true_term  = terms[0];  // abs(abs(var(1)))
    const lc_expr* false_term = terms[1];  // abs(abs(var(0)))

    globals.push(true_term);
    globals.push(false_term);

    // app(abs(app(abs(nullptr), false_term)), true_term)
    // true_term is outermost: its binding is var(1) from inside false_term's scope.
    const lc_expr* expected =
        lc.make_app(lc.make_abs(lc.make_app(lc.make_abs(nullptr), false_term)), true_term);
    EXPECT_EQ(asm_.assemble(), expected);
}

// ---------------------------------------------------------------------------
// BooleanDeclarationsAndNotDefinition
//
// Full pipeline via aml_runtime: {true/0, false/0} + not = a => a false true.
// assemble() must return a three-binding let-chain:
//   app(abs(app(abs(app(abs(nullptr), not_lc)), false_term)), true_term)
// ---------------------------------------------------------------------------

TEST(AssemblerIntegrationTest, BooleanDeclarationsAndNotDefinition) {
    aml_expr_pool aml;
    aml_runtime   rt;

    rt.visit_declaration_group(make_group({{"true", 0u}, {"false", 0u}}));

    const aml_expr* not_body = aml.make_abs("a",
        aml.make_app(
            aml.make_app(aml.make_token("a"), aml.make_token("false")),
            aml.make_token("true")));
    rt.visit_definition({"not", not_body});

    const lc_expr* result = rt.assemble();
    ASSERT_NE(result, nullptr);

    // Three-level let-chain: outermost arg = true_term = abs(abs(var(1)))
    ASSERT_TRUE(std::holds_alternative<lc_expr::app>(result->content));
    const auto& outer = std::get<lc_expr::app>(result->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.func->content));

    // true_term structure: abs(abs(var(1)))
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.arg->content));
    const auto& true_outer_abs = std::get<lc_expr::abs>(outer.arg->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(true_outer_abs.body->content));
    const auto& true_inner_abs = std::get<lc_expr::abs>(true_outer_abs.body->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::var>(true_inner_abs.body->content));
    EXPECT_EQ(std::get<lc_expr::var>(true_inner_abs.body->content).index, 1u);
}
