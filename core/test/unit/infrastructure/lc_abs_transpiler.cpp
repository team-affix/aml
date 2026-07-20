#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/lc_abs_transpiler.hpp"
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

struct LcAbsTranspilerTest : public ::testing::Test {
    NiceTx  mock_tx;
    NiceFun mock_fun;
    lc_abs_transpiler<NiceTx, NiceFun> tx{mock_tx, mock_fun};
    lc_expr body{};
    expr body_chc{};
    expr abs_term{};
};

} // namespace

TEST_F(LcAbsTranspilerTest, WrapsTranspiledBody) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_tx, transpile(&body)).WillOnce(Return(&body_chc));
        EXPECT_CALL(mock_fun, make_functor(k_abs_functor_id,
            std::vector<const expr*>{&body_chc}))
            .WillOnce(Return(&abs_term));
    }
    EXPECT_EQ(tx.transpile_abs(lc_expr::abs{&body}), &abs_term);
}
