#include <gtest/gtest.h>
#include "infrastructure/lc_expr_eq.hpp"
#include "infrastructure/lc_expr_pool.hpp"

TEST(LcExprEqTest, PointerIdentity) {
    lc_expr_pool pool;
    const lc_expr* x = pool.make_var(0);
    EXPECT_TRUE(lc_expr_eq(x, x));
}

TEST(LcExprEqTest, SameStructureDifferentPointers) {
    lc_expr_pool a;
    lc_expr_pool b;
    const lc_expr* ax = a.make_var(0);
    const lc_expr* bx = b.make_var(0);
    EXPECT_TRUE(lc_expr_eq(ax, bx));
}

TEST(LcExprEqTest, DifferentStructure) {
    lc_expr_pool pool;
    EXPECT_FALSE(lc_expr_eq(pool.make_var(0), pool.make_var(1)));
}

TEST(LcExprEqTest, NestedStructure) {
    lc_expr_pool pool;
    const lc_expr* id = pool.make_abs(pool.make_var(0));
    const lc_expr* app = pool.make_app(id, pool.make_var(0));
    EXPECT_TRUE(lc_expr_eq(app, pool.make_app(id, pool.make_var(0))));
}
