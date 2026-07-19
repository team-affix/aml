// Integration tests: assembler + lc_expr_pool.
//
// These tests verify the structural correctness of assemble() using real
// lc_expr_pool nodes, and confirm that the assembler's definition-order
// invariant matches the scope's de Bruijn assignment.

#include <gtest/gtest.h>
#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/lc_transpiler_manifest.hpp"
#include "value_objects/assembler_manifest.hpp"

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
// Full pipeline using split manifests: declaration_transpiler + lc_transpiler_manifest
// for transpilation, assembler_manifest for assembly.
//
// Stack pushed in definition order: true (bottom), false, not (top).
// After pushing all three names to scope (depth 3):
//   "not"   → var(0)
//   "false" → var(1)
//   "true"  → var(2)
//
// not = a => a false true
//   → abs(app(app(var(0), var(1)), var(2)))
//       a=var(0), false=var(1), true=var(2) inside the abs
//
// assemble():
//   pop not_lc   → step1 = app(abs(nullptr),  not_lc)    [wrappers from asm_m.lc]
//   pop false    → step2 = app(abs(step1),     false_term)
//   pop true     → step3 = app(abs(step2),     true_term) ← true_term outermost
// ---------------------------------------------------------------------------

TEST(AssemblerIntegrationTest, BooleanDeclarationsAndNotDefinition) {
    aml_expr_pool          aml;
    lc_transpiler_manifest lc_m;
    assembler_manifest     asm_m;

    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc_m.lc, lc_m.lc, lc_m.lc};

    // true/0 | false/0  — terms live in lc_m.lc
    auto terms = dt.transpile_group(make_group({{"true", 0u}, {"false", 0u}}));
    const lc_expr* true_term  = terms[0];
    const lc_expr* false_term = terms[1];

    // Push in definition order so the transpiler sees the right indices.
    lc_m.sc.push("true");
    lc_m.sc.push("false");

    // not = a => a false true
    const aml_expr* not_body = aml.make_abs("a",
        aml.make_app(
            aml.make_app(aml.make_token("a"), aml.make_token("false")),
            aml.make_token("true")));
    const lc_expr* not_lc = lc_m.tx.transpile(not_body);

    lc_m.sc.push("not");

    // Verify the transpiled not body before assembly.
    // Inside abs("a"): a=var(0), false=var(1), true=var(2).  Uses lc_m.lc for comparison.
    EXPECT_EQ(not_lc,
              lc_m.lc.make_abs(
                  lc_m.lc.make_app(
                      lc_m.lc.make_app(lc_m.lc.make_var(0), lc_m.lc.make_var(1)),
                      lc_m.lc.make_var(2))));

    // Clean up scope (assembler does not touch it).
    lc_m.sc.pop(); lc_m.sc.pop(); lc_m.sc.pop();

    // Push globals onto assembler stage in definition order, then assemble.
    // asm_m.lc creates the app/abs wrapper nodes; inner terms come from lc_m.lc.
    asm_m.globals.push(true_term);
    asm_m.globals.push(false_term);
    asm_m.globals.push(not_lc);

    const lc_expr* step1 = asm_m.lc.make_app(asm_m.lc.make_abs(nullptr),  not_lc);
    const lc_expr* step2 = asm_m.lc.make_app(asm_m.lc.make_abs(step1),    false_term);
    const lc_expr* step3 = asm_m.lc.make_app(asm_m.lc.make_abs(step2),    true_term);

    EXPECT_EQ(asm_m.asm_.assemble(), step3);
}
