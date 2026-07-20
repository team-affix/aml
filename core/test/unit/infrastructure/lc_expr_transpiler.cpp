#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/lc_expr_transpiler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor,
                (uint32_t, (const std::vector<const expr*>&)), ());
};

using NiceVar = testing::NiceMock<MockMakeVar>;
using NiceFun = testing::NiceMock<MockMakeFunctor>;

struct LcExprTranspilerTest : public ::testing::Test {
    using lc_tx_t = lc_expr_transpiler<NiceVar, NiceFun>;
    using lc_var_transpiler_t     = typename lc_tx_t::lc_var_transpiler_t;
    using lc_abs_transpiler_t     = typename lc_tx_t::lc_abs_transpiler_t;
    using lc_app_transpiler_t     = typename lc_tx_t::lc_app_transpiler_t;
    using lc_nullptr_transpiler_t = typename lc_tx_t::lc_nullptr_transpiler_t;

    NiceVar mock_var;
    NiceFun mock_fun;

    lc_tx_t                   lc_tx;
    lc_var_transpiler_t       lc_var_;
    lc_abs_transpiler_t       lc_abs_;
    lc_app_transpiler_t       lc_app_;
    lc_nullptr_transpiler_t   lc_nullptr_;

    expr hole{};
    expr zero{};
    expr var_term{};
    expr abs_term{};

    LcExprTranspilerTest()
        : lc_tx(lc_var_, lc_abs_, lc_app_, lc_nullptr_)
        , lc_var_(mock_fun)
        , lc_abs_(lc_tx, mock_fun)
        , lc_app_(lc_tx, mock_fun)
        , lc_nullptr_(mock_var) {}
};

} // namespace

TEST_F(LcExprTranspilerTest, NullptrDelegatesToNullptrTranspiler) {
    using testing::Return;

    EXPECT_CALL(mock_var, make_var(0u)).WillOnce(Return(&hole));
    EXPECT_EQ(lc_tx.transpile(nullptr), &hole);
}

TEST_F(LcExprTranspilerTest, VarDelegatesToVarTranspiler) {
    using testing::Return;
    using testing::InSequence;

    lc_expr v{};
    v.content = lc_expr::var{0};
    {
        InSequence seq;
        EXPECT_CALL(mock_fun, make_functor(k_zero_functor_id,
            std::vector<const expr*>{}))
            .WillOnce(Return(&zero));
        EXPECT_CALL(mock_fun, make_functor(k_var_functor_id,
            std::vector<const expr*>{&zero}))
            .WillOnce(Return(&var_term));
    }
    EXPECT_EQ(lc_tx.transpile(&v), &var_term);
}

TEST_F(LcExprTranspilerTest, AbsDelegatesToAbsTranspiler) {
    using testing::Return;
    using testing::InSequence;

    lc_expr body{};
    body.content = lc_expr::var{0};
    lc_expr a{};
    a.content = lc_expr::abs{&body};

    {
        InSequence seq;
        EXPECT_CALL(mock_fun, make_functor(k_zero_functor_id,
            std::vector<const expr*>{}))
            .WillOnce(Return(&zero));
        EXPECT_CALL(mock_fun, make_functor(k_var_functor_id,
            std::vector<const expr*>{&zero}))
            .WillOnce(Return(&var_term));
        EXPECT_CALL(mock_fun, make_functor(k_abs_functor_id,
            std::vector<const expr*>{&var_term}))
            .WillOnce(Return(&abs_term));
    }
    EXPECT_EQ(lc_tx.transpile(&a), &abs_term);
}
