// Integration tests: declaration_transpiler + transpiler_manifest + global_wrapper.
//
// These tests verify that the three core assembly components work together
// correctly to build closed lambda-calculus programs from declaration groups
// and definition bodies.
//
// declaration_group values are constructed directly (no parser dependency).
// This file replaces parser/test/integration/program_assembly.cpp.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/global_wrapper.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/transpiler_manifest.hpp"

namespace {

static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

} // namespace

// ---------------------------------------------------------------------------
// declaration_transpiler + lc_expr_pool (real components, no transpiler_manifest)
// ---------------------------------------------------------------------------

TEST(ProgramAssemblyIntegrationTest, DeclarationGroupThreeConstructors) {
    lc_expr_pool lc;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};

    auto pairs = dt.transpile_group(make_group({{"abc", 0u}, {"def", 0u}, {"ghi", 1u}}));
    ASSERT_EQ(pairs.size(), 3u);
    EXPECT_EQ(pairs[0].first, "abc");
    EXPECT_EQ(pairs[1].first, "def");
    EXPECT_EQ(pairs[2].first, "ghi");

    // abc (k=0, a=0, n=3): abs(abs(abs(var(2))))
    EXPECT_EQ(pairs[0].second,
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(2)))));
    // def (k=1, a=0, n=3): abs(abs(abs(var(1))))
    EXPECT_EQ(pairs[1].second,
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(1)))));
    // ghi (k=2, a=1, n=3): abs(abs(abs(abs(app(var(1), var(0))))))
    EXPECT_EQ(pairs[2].second,
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_abs(
                  lc.make_app(lc.make_var(1), lc.make_var(0)))))));
}

TEST(ProgramAssemblyIntegrationTest, BooleanDeclarations) {
    lc_expr_pool lc;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};

    auto pairs = dt.transpile_group(make_group({{"true", 0u}, {"false", 0u}}));
    ASSERT_EQ(pairs.size(), 2u);
    // true (k=0, a=0, n=2): abs(abs(var(1)))
    EXPECT_EQ(pairs[0].second, lc.make_abs(lc.make_abs(lc.make_var(1))));
    // false (k=1, a=0, n=2): abs(abs(var(0)))
    EXPECT_EQ(pairs[1].second, lc.make_abs(lc.make_abs(lc.make_var(0))));
}

// ---------------------------------------------------------------------------
// global_wrapper + lc_expr_pool
// ---------------------------------------------------------------------------

TEST(ProgramAssemblyIntegrationTest, GlobalWrapperEmptyGlobals) {
    lc_expr_pool lc;
    global_wrapper<lc_expr_pool, lc_expr_pool> gw{lc, lc};
    const lc_expr* main = lc.make_var(0);
    EXPECT_EQ(gw.wrap(main, {}), main);
}

TEST(ProgramAssemblyIntegrationTest, GlobalWrapperTwoGlobals) {
    lc_expr_pool lc;
    global_wrapper<lc_expr_pool, lc_expr_pool> gw{lc, lc};
    const lc_expr* main = lc.make_var(99);
    const lc_expr* g0   = lc.make_var(0);
    const lc_expr* g1   = lc.make_var(1);

    // wrap iterates in reverse (g1 innermost, g0 outermost):
    // app(abs(app(abs(main), g1)), g0)
    const lc_expr* step1 = lc.make_app(lc.make_abs(main), g1);
    const lc_expr* step2 = lc.make_app(lc.make_abs(step1), g0);
    EXPECT_EQ(gw.wrap(main, {g0, g1}), step2);
}

// ---------------------------------------------------------------------------
// Full pipeline: declaration_transpiler + transpiler_manifest + global_wrapper
// ---------------------------------------------------------------------------

TEST(ProgramAssemblyIntegrationTest, MainReferencingDeclaredGlobals) {
    aml_expr_pool aml;
    transpiler_manifest b;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{b.lc, b.lc, b.lc};

    // Declarations: abc/0 | def/0 | ghi/1
    auto ctor_pairs = dt.transpile_group(
        make_group({{"abc", 0u}, {"def", 0u}, {"ghi", 1u}}));

    std::vector<const lc_expr*> global_terms;
    for (const auto& [name, term] : ctor_pairs) {
        b.sc.push(name);
        global_terms.push_back(term);
    }

    // main = ghi abc
    const aml_expr* main_body =
        aml.make_app(aml.make_token("ghi"), aml.make_token("abc"));
    const lc_expr* main_lc = b.tx.transpile(main_body);

    // depth=3: ghi→0, def→1, abc→2
    EXPECT_EQ(main_lc, b.lc.make_app(b.lc.make_var(0), b.lc.make_var(2)));

    for (size_t i = 0; i < global_terms.size(); ++i) b.sc.pop();

    global_wrapper<lc_expr_pool, lc_expr_pool> gw{b.lc, b.lc};
    const lc_expr* program = gw.wrap(main_lc, global_terms);

    // wrap reverses: ghi(idx2) innermost, def(idx1) middle, abc(idx0) outermost
    const lc_expr* s1 = b.lc.make_app(b.lc.make_abs(main_lc), ctor_pairs[2].second);
    const lc_expr* s2 = b.lc.make_app(b.lc.make_abs(s1),      ctor_pairs[1].second);
    const lc_expr* s3 = b.lc.make_app(b.lc.make_abs(s2),      ctor_pairs[0].second);
    EXPECT_EQ(program, s3);
}

TEST(ProgramAssemblyIntegrationTest, NotFunctionWrappedWithBooleans) {
    aml_expr_pool aml;
    transpiler_manifest b;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{b.lc, b.lc, b.lc};

    // true/0 | false/0
    auto pairs = dt.transpile_group(make_group({{"true", 0u}, {"false", 0u}}));
    EXPECT_EQ(pairs[0].second, b.lc.make_abs(b.lc.make_abs(b.lc.make_var(1))));
    EXPECT_EQ(pairs[1].second, b.lc.make_abs(b.lc.make_abs(b.lc.make_var(0))));

    for (const auto& [name, _] : pairs) b.sc.push(name);

    // not = b => b false true
    const aml_expr* not_body = aml.make_abs("b",
        aml.make_app(
            aml.make_app(aml.make_token("b"), aml.make_token("false")),
            aml.make_token("true")));
    const lc_expr* not_lc = b.tx.transpile(not_body);

    for (size_t i = 0; i < pairs.size(); ++i) b.sc.pop();

    global_wrapper<lc_expr_pool, lc_expr_pool> gw{b.lc, b.lc};
    std::vector<const lc_expr*> globals = {pairs[0].second, pairs[1].second};
    const lc_expr* program = gw.wrap(not_lc, globals);

    // wrap reversed: false(idx1) innermost, true(idx0) outermost
    // app(abs(app(abs(not_lc), false_term)), true_term)
    const lc_expr* expected =
        b.lc.make_app(
            b.lc.make_abs(
                b.lc.make_app(b.lc.make_abs(not_lc), pairs[1].second)),
            pairs[0].second);
    EXPECT_EQ(program, expected);
}

TEST(ProgramAssemblyIntegrationTest, TranspilerManifestScopeIsShared) {
    // Verifies that the scope pushed for globals is the same scope used
    // during body transpilation — i.e. sc and tx are wired to the same object.
    aml_expr_pool aml;
    transpiler_manifest b;

    b.sc.push("g0");
    b.sc.push("g1");
    // After two pushes, "g1" has index 0, "g0" has index 1
    EXPECT_EQ(b.sc.get_var_index("g1"), 0u);
    EXPECT_EQ(b.sc.get_var_index("g0"), 1u);

    // Transpiling a token should see the same scope state
    const aml_expr* e = aml.make_token("g1");
    EXPECT_EQ(b.tx.transpile(e), b.lc.make_var(0));

    b.sc.pop();
    b.sc.pop();
}
