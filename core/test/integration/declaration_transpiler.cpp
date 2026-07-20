// Integration tests: declaration_transpiler + lc_expr_pool encodings.

#include <gtest/gtest.h>
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct DeclarationTranspilerIntegrationTest : public ::testing::Test {
    lc_expr_pool lc;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
};

} // namespace

TEST_F(DeclarationTranspilerIntegrationTest, SingleVariantArity0) {
    EXPECT_EQ(dt.transpile_decl(1u, 0u, 0u), lc.make_abs(lc.make_var(0)));
}

TEST_F(DeclarationTranspilerIntegrationTest, BooleanDeclarations) {
    EXPECT_EQ(dt.transpile_decl(2u, 0u, 0u),
              lc.make_abs(lc.make_abs(lc.make_var(1))));
    EXPECT_EQ(dt.transpile_decl(2u, 1u, 0u),
              lc.make_abs(lc.make_abs(lc.make_var(0))));
}

TEST_F(DeclarationTranspilerIntegrationTest, DeclarationGroupThreeConstructors) {
    EXPECT_EQ(dt.transpile_decl(3u, 0u, 0u),
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(2)))));
    EXPECT_EQ(dt.transpile_decl(3u, 1u, 0u),
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(1)))));
    EXPECT_EQ(dt.transpile_decl(3u, 2u, 1u),
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_abs(
                  lc.make_app(lc.make_var(1), lc.make_var(0)))))));
}

TEST_F(DeclarationTranspilerIntegrationTest, ListEncoding) {
    EXPECT_EQ(dt.transpile_decl(2u, 0u, 0u),
              lc.make_abs(lc.make_abs(lc.make_var(1))));
    EXPECT_EQ(dt.transpile_decl(2u, 1u, 2u),
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_abs(
                  lc.make_app(lc.make_app(lc.make_var(2), lc.make_var(1)),
                              lc.make_var(0)))))));
}

TEST_F(DeclarationTranspilerIntegrationTest, HighAritySingleVariant) {
    EXPECT_EQ(dt.transpile_decl(1u, 0u, 3u),
              lc.make_abs(lc.make_abs(lc.make_abs(lc.make_abs(
                  lc.make_app(lc.make_app(lc.make_app(lc.make_var(3), lc.make_var(2)),
                                          lc.make_var(1)),
                              lc.make_var(0)))))));
}
