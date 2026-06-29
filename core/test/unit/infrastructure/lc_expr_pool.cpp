#include <gtest/gtest.h>
#include "infrastructure/lc_expr_pool.hpp"

struct LcExprPoolTest : public ::testing::Test {
    lc_expr_pool pool;
};

TEST_F(LcExprPoolTest, VarInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_var(0), pool.make_var(0));
}

TEST_F(LcExprPoolTest, DifferentVarsReturnDifferentPointers) {
    EXPECT_NE(pool.make_var(0), pool.make_var(1));
}

TEST_F(LcExprPoolTest, AbsInternedTwiceReturnsSamePointer) {
    const lc_expr* body = pool.make_var(0);
    EXPECT_EQ(pool.make_abs(body), pool.make_abs(body));
}

TEST_F(LcExprPoolTest, AppInternedTwiceReturnsSamePointer) {
    const lc_expr* f = pool.make_var(1);
    const lc_expr* x = pool.make_var(0);
    EXPECT_EQ(pool.make_app(f, x), pool.make_app(f, x));
}

TEST_F(LcExprPoolTest, DifferentAppsReturnDifferentPointers) {
    const lc_expr* f = pool.make_var(2);
    const lc_expr* x = pool.make_var(0);
    const lc_expr* y = pool.make_var(1);
    EXPECT_NE(pool.make_app(f, x), pool.make_app(f, y));
}


TEST_F(LcExprPoolTest, AbsDifferentBodiesReturnDifferentPointers) {
    const lc_expr* b0 = pool.make_var(0);
    const lc_expr* b1 = pool.make_var(1);
    EXPECT_NE(pool.make_abs(b0), pool.make_abs(b1));
}

TEST_F(LcExprPoolTest, AppSameFuncDifferentArgReturnsDifferentPointers) {
    const lc_expr* f = pool.make_var(2);
    const lc_expr* x = pool.make_var(0);
    const lc_expr* y = pool.make_var(1);
    EXPECT_NE(pool.make_app(f, x), pool.make_app(f, y));
}

TEST_F(LcExprPoolTest, AppDifferentFuncSameArgReturnsDifferentPointers) {
    const lc_expr* f = pool.make_var(1);
    const lc_expr* g = pool.make_var(2);
    const lc_expr* x = pool.make_var(0);
    EXPECT_NE(pool.make_app(f, x), pool.make_app(g, x));
}

TEST_F(LcExprPoolTest, AppSwappedFuncArgReturnsDifferentPointers) {
    const lc_expr* a = pool.make_var(0);
    const lc_expr* b = pool.make_var(1);
    EXPECT_NE(pool.make_app(a, b), pool.make_app(b, a));
}

