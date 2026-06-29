// Integration tests: declaration_transpiler + lc_expr_pool.
//
// Tests of the full declaration_transpiler → assembler pipeline live in
// core/test/integration/assembler.cpp.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"

namespace {

static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

} // namespace

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
