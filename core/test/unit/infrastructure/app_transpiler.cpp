#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_transpiler.hpp"
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile,  (const aml_expr*)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app,   (const lc_expr*, const lc_expr*)); };

struct AppTranspilerTest : public ::testing::Test {
    aml_expr_pool aml;
    lc_expr_pool  lc;

    MockTranspileExpr mock_tx;
    MockMakeLcApp     mock_app;

    app_transpiler<MockTranspileExpr, MockMakeLcApp> apt{mock_tx, mock_app};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
    }
};

} // namespace

// The observable contract:
//  1. transpile(func) and transpile(arg) are both called.
//  2. make_app receives (transpile(func), transpile(arg)) — in that order.
//  3. The result of make_app is returned.

TEST_F(AppTranspilerTest, TranspilesBothSubexpressionsAndCombines) {
    using testing::Return;
    using testing::_;

    const aml_expr* aml_f = aml.make_token("f");
    const aml_expr* aml_x = aml.make_token("x");
    const lc_expr*  lc_f  = lc.make_var(1);
    const lc_expr*  lc_x  = lc.make_var(0);

    EXPECT_CALL(mock_tx, transpile(aml_f)).Times(1).WillOnce(Return(lc_f));
    EXPECT_CALL(mock_tx, transpile(aml_x)).Times(1).WillOnce(Return(lc_x));
    EXPECT_CALL(mock_app, make_app(lc_f, lc_x)).Times(1);

    apt.transpile_app(aml_expr::app{aml_f, aml_x});
}

TEST_F(AppTranspilerTest, ReturnsResultOfMakeApp) {
    using testing::Return;
    using testing::_;

    const aml_expr* aml_f = aml.make_token("f");
    const aml_expr* aml_x = aml.make_token("x");
    const lc_expr*  lc_f  = lc.make_var(1);
    const lc_expr*  lc_x  = lc.make_var(0);
    const lc_expr*  lc_fx = lc.make_app(lc_f, lc_x);

    ON_CALL(mock_tx, transpile(aml_f)).WillByDefault(Return(lc_f));
    ON_CALL(mock_tx, transpile(aml_x)).WillByDefault(Return(lc_x));
    ON_CALL(mock_app, make_app(_, _)).WillByDefault(Return(lc_fx));

    EXPECT_EQ(apt.transpile_app(aml_expr::app{aml_f, aml_x}), lc_fx);
}

TEST_F(AppTranspilerTest, FuncAndArgPassedInCorrectOrder) {
    // Verify that the func result is the first argument to make_app
    // and the arg result is the second.  Swapping them is a silent bug
    // for asymmetric applications.
    using testing::Return;
    using testing::_;

    const aml_expr* aml_f = aml.make_token("f");
    const aml_expr* aml_a = aml.make_token("a");
    const lc_expr*  lc_f  = lc.make_var(2);
    const lc_expr*  lc_a  = lc.make_var(3);

    ON_CALL(mock_tx, transpile(aml_f)).WillByDefault(Return(lc_f));
    ON_CALL(mock_tx, transpile(aml_a)).WillByDefault(Return(lc_a));

    // make_app(lc_f, lc_a) ≠ make_app(lc_a, lc_f) — the pool distinguishes them
    EXPECT_EQ(apt.transpile_app(aml_expr::app{aml_f, aml_a}),
              lc.make_app(lc_f, lc_a));
}
