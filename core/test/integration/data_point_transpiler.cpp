// Integration: real expr_pool + lc_expr_transpiler + data_point_transpiler.

#include <gtest/gtest.h>
#include "infrastructure/data_point_transpiler.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/lc_expr_transpiler.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/data_point.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct DataPointTranspilerIntegrationTest : public ::testing::Test {
    using lc_tx_t = lc_expr_transpiler<expr_pool, expr_pool>;
    using lc_var_transpiler_t     = typename lc_tx_t::lc_var_transpiler_t;
    using lc_abs_transpiler_t     = typename lc_tx_t::lc_abs_transpiler_t;
    using lc_app_transpiler_t     = typename lc_tx_t::lc_app_transpiler_t;
    using lc_nullptr_transpiler_t = typename lc_tx_t::lc_nullptr_transpiler_t;
    using data_point_transpiler_t =
        data_point_transpiler<lc_tx_t, expr_pool, expr_pool>;

    expr_pool                 chc;
    lc_expr_pool              lc;
    lc_tx_t                   lc_tx;
    lc_var_transpiler_t       lc_var_;
    lc_abs_transpiler_t       lc_abs_;
    lc_app_transpiler_t       lc_app_;
    lc_nullptr_transpiler_t   lc_nullptr_;
    data_point_transpiler_t   data_point_tx;

    DataPointTranspilerIntegrationTest()
        : lc_tx(lc_var_, lc_abs_, lc_app_, lc_nullptr_)
        , lc_var_(chc)
        , lc_abs_(lc_tx, chc)
        , lc_app_(lc_tx, chc)
        , lc_nullptr_(chc)
        , data_point_tx(lc_tx, chc, chc) {}

    const expr* peano(uint32_t n) {
        const expr* r = chc.make_functor(k_zero_functor_id, {});
        for (uint32_t i = 0; i < n; ++i)
            r = chc.make_functor(k_suc_functor_id, {r});
        return r;
    }

    const expr* dv(uint32_t n) {
        return chc.make_functor(k_var_functor_id, {peano(n)});
    }
};

} // namespace

TEST_F(DataPointTranspilerIntegrationTest, NormalizeAppModelInputLabel) {
    const lc_expr* x = lc.make_var(0);
    const lc_expr* y = lc.make_var(1);
    data_point dp{x, y};

    const expr* x_chc = dv(0);
    const expr* y_chc = dv(1);
    const expr* M     = chc.make_var(k_model_var_id);
    const expr* expected = chc.make_functor(
        k_normalize_functor_id,
        {chc.make_functor(k_app_functor_id, {M, x_chc}), y_chc});

    EXPECT_EQ(data_point_tx.transpile_data_point(dp), expected);
}
