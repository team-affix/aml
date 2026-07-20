#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/lc_app_transpiler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct MockTranspileLcExpr {
    MOCK_METHOD(const expr*, transpile, (const lc_expr*), ());
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor,
                (uint32_t, (const std::vector<const expr*>&)), ());
};

using NiceTx  = testing::NiceMock<MockTranspileLcExpr>;
using NiceFun = testing::NiceMock<MockMakeFunctor>;

struct LcAppTranspilerTest : public ::testing::Test {
    NiceTx  mock_tx;
    NiceFun mock_fun;
    lc_app_transpiler<NiceTx, NiceFun> tx{mock_tx, mock_fun};
    lc_expr func{};
    lc_expr arg{};
    expr func_chc{};
    expr arg_chc{};
    expr app_term{};
};

} // namespace

TEST_F(LcAppTranspilerTest, WrapsTranspiledFuncAndArg) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_tx, transpile(&func)).WillOnce(Return(&func_chc));
        EXPECT_CALL(mock_tx, transpile(&arg)).WillOnce(Return(&arg_chc));
        EXPECT_CALL(mock_fun, make_functor(k_app_functor_id,
            std::vector<const expr*>{&func_chc, &arg_chc}))
            .WillOnce(Return(&app_term));
    }
    EXPECT_EQ(tx.transpile_app(lc_expr::app{&func, &arg}), &app_term);
}
