// Integration: real expr_pool + full lc_expr_transpiler stack.

#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/lc_expr_transpiler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct LcExprTranspilerIntegrationTest : public ::testing::Test {
    using lc_tx_t = lc_expr_transpiler<expr_pool, expr_pool>;
    using lc_var_transpiler_t     = typename lc_tx_t::lc_var_transpiler_t;
    using lc_abs_transpiler_t     = typename lc_tx_t::lc_abs_transpiler_t;
    using lc_app_transpiler_t     = typename lc_tx_t::lc_app_transpiler_t;
    using lc_nullptr_transpiler_t = typename lc_tx_t::lc_nullptr_transpiler_t;

    expr_pool                 chc;
    lc_expr_pool              lc;
    lc_tx_t                   lc_tx;
    lc_var_transpiler_t       lc_var_;
    lc_abs_transpiler_t       lc_abs_;
    lc_app_transpiler_t       lc_app_;
    lc_nullptr_transpiler_t   lc_nullptr_;

    LcExprTranspilerIntegrationTest()
        : lc_tx(lc_var_, lc_abs_, lc_app_, lc_nullptr_)
        , lc_var_(chc)
        , lc_abs_(lc_tx, chc)
        , lc_app_(lc_tx, chc)
        , lc_nullptr_(chc) {}

    const expr* peano(uint32_t n) {
        const expr* r = chc.make_functor(k_zero_functor_id, {});
        for (uint32_t i = 0; i < n; ++i)
            r = chc.make_functor(k_suc_functor_id, {r});
        return r;
    }

    const expr* dv(uint32_t n) {
        return chc.make_functor(k_var_functor_id, {peano(n)});
    }

    const expr* lm(const expr* body) {
        return chc.make_functor(k_abs_functor_id, {body});
    }

    const expr* ap(const expr* f, const expr* a) {
        return chc.make_functor(k_app_functor_id, {f, a});
    }
};

} // namespace

TEST_F(LcExprTranspilerIntegrationTest, NullptrIsVarZero) {
    EXPECT_EQ(lc_tx.transpile(nullptr), chc.make_var(0));
}

TEST_F(LcExprTranspilerIntegrationTest, VarZero) {
    const lc_expr* v = lc.make_var(0);
    EXPECT_EQ(lc_tx.transpile(v), dv(0));
}

TEST_F(LcExprTranspilerIntegrationTest, VarTwo) {
    const lc_expr* v = lc.make_var(2);
    EXPECT_EQ(lc_tx.transpile(v), dv(2));
}

TEST_F(LcExprTranspilerIntegrationTest, AbsVarZero) {
    const lc_expr* term = lc.make_abs(lc.make_var(0));
    EXPECT_EQ(lc_tx.transpile(term), lm(dv(0)));
}

TEST_F(LcExprTranspilerIntegrationTest, AppAbsNullptr) {
    // abs(nullptr) applied to var(0) — nullptr becomes solver var 0
    const lc_expr* term = lc.make_app(lc.make_abs(nullptr), lc.make_var(0));
    EXPECT_EQ(lc_tx.transpile(term), ap(lm(chc.make_var(0)), dv(0)));
}
