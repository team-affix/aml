#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/data_point_transpiler.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/data_point.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct MockTranspileLcExpr {
    MOCK_METHOD(const expr*, transpile, (const lc_expr*), ());
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor,
                (uint32_t, (const std::vector<const expr*>&)), ());
};

using NiceTx  = testing::NiceMock<MockTranspileLcExpr>;
using NiceVar = testing::NiceMock<MockMakeVar>;
using NiceFun = testing::NiceMock<MockMakeFunctor>;

struct DataPointTranspilerTest : public ::testing::Test {
    NiceTx  mock_tx;
    NiceVar mock_var;
    NiceFun mock_fun;
    data_point_transpiler<NiceTx, NiceVar, NiceFun> tx{mock_tx, mock_var, mock_fun};
    lc_expr input{};
    lc_expr label{};
    expr x{};
    expr y{};
    expr M{};
    expr applied{};
    expr goal{};
};

} // namespace

TEST_F(DataPointTranspilerTest, BuildsNormalizeAppModelInputLabel) {
    using testing::Return;
    using testing::InSequence;

    data_point dp{&input, &label};
    {
        InSequence seq;
        EXPECT_CALL(mock_tx, transpile(&input)).WillOnce(Return(&x));
        EXPECT_CALL(mock_tx, transpile(&label)).WillOnce(Return(&y));
        EXPECT_CALL(mock_var, make_var(k_model_var_id)).WillOnce(Return(&M));
        EXPECT_CALL(mock_fun, make_functor(k_app_functor_id,
            std::vector<const expr*>{&M, &x}))
            .WillOnce(Return(&applied));
        EXPECT_CALL(mock_fun, make_functor(k_normalize_functor_id,
            std::vector<const expr*>{&applied, &y}))
            .WillOnce(Return(&goal));
    }
    EXPECT_EQ(tx.transpile_data_point(dp), &goal);
}
