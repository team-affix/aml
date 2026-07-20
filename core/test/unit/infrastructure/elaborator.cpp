#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/elaborator.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/data_point.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct MockPumpGlobals {
    MOCK_METHOD(void, pump, (), ());
};
struct MockPumpStatements {
    MOCK_METHOD(void, pump, (), ());
};
struct MockAssemble {
    MOCK_METHOD(const lc_expr*, assemble, (), ());
};
struct MockTranspileLcExpr {
    MOCK_METHOD(const expr*, transpile, (const lc_expr*), ());
};
struct MockGetNextDataPoint {
    MOCK_METHOD(std::optional<data_point>, get_next_data_point, (), ());
};
struct MockTranspileDataPoint {
    MOCK_METHOD(const expr*, transpile_data_point, (const data_point&), ());
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor,
                (uint32_t, (const std::vector<const expr*>&)), ());
};
struct MockPushInitialGoal {
    MOCK_METHOD(void, push, (const expr*), ());
};

using StrictPumpG = testing::StrictMock<MockPumpGlobals>;
using StrictPumpS = testing::StrictMock<MockPumpStatements>;
using StrictAsm   = testing::StrictMock<MockAssemble>;
using StrictTx    = testing::StrictMock<MockTranspileLcExpr>;
using StrictGet   = testing::StrictMock<MockGetNextDataPoint>;
using StrictDpTx  = testing::StrictMock<MockTranspileDataPoint>;
using StrictVar   = testing::StrictMock<MockMakeVar>;
using StrictFun   = testing::StrictMock<MockMakeFunctor>;
using StrictPush  = testing::StrictMock<MockPushInitialGoal>;

struct ElaboratorTest : public ::testing::Test {
    StrictPumpG pump_g;
    StrictPumpS pump_s;
    StrictAsm   assemble;
    StrictTx    tx;
    StrictGet   get_dp;
    StrictDpTx  dp_tx;
    StrictVar   make_var;
    StrictFun   make_fun;
    StrictPush  push;

    elaborator<StrictPumpG, StrictPumpS, StrictAsm, StrictTx, StrictGet,
               StrictDpTx, StrictVar, StrictFun, StrictPush>
        elab{pump_g, pump_s, assemble, tx, get_dp, dp_tx, make_var, make_fun, push};

    lc_expr program{};
    expr P{};
    expr M{};
    expr eq_goal{};
    lc_expr input{};
    lc_expr label{};
    expr dp_goal{};
};

} // namespace

TEST_F(ElaboratorTest, ElaboratePushesEqThenDataPointGoalsInOrder) {
    using testing::Return;
    using testing::InSequence;

    data_point dp{&input, &label};
    {
        InSequence seq;
        EXPECT_CALL(pump_g, pump());
        EXPECT_CALL(pump_s, pump());
        EXPECT_CALL(assemble, assemble()).WillOnce(Return(&program));
        EXPECT_CALL(tx, transpile(&program)).WillOnce(Return(&P));
        EXPECT_CALL(make_var, make_var(k_model_var_id)).WillOnce(Return(&M));
        EXPECT_CALL(make_fun, make_functor(k_eq_functor_id,
            std::vector<const expr*>{&M, &P}))
            .WillOnce(Return(&eq_goal));
        EXPECT_CALL(push, push(&eq_goal));
        EXPECT_CALL(get_dp, get_next_data_point())
            .WillOnce(Return(std::optional<data_point>{dp}));
        EXPECT_CALL(dp_tx, transpile_data_point(dp)).WillOnce(Return(&dp_goal));
        EXPECT_CALL(push, push(&dp_goal));
        EXPECT_CALL(get_dp, get_next_data_point())
            .WillOnce(Return(std::nullopt));
    }
    elab.elaborate();
}

TEST_F(ElaboratorTest, ElaborateWithNoDataPointsStillPushesEqGoal) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(pump_g, pump());
        EXPECT_CALL(pump_s, pump());
        EXPECT_CALL(assemble, assemble()).WillOnce(Return(&program));
        EXPECT_CALL(tx, transpile(&program)).WillOnce(Return(&P));
        EXPECT_CALL(make_var, make_var(k_model_var_id)).WillOnce(Return(&M));
        EXPECT_CALL(make_fun, make_functor(k_eq_functor_id,
            std::vector<const expr*>{&M, &P}))
            .WillOnce(Return(&eq_goal));
        EXPECT_CALL(push, push(&eq_goal));
        EXPECT_CALL(get_dp, get_next_data_point())
            .WillOnce(Return(std::nullopt));
    }
    elab.elaborate();
}
