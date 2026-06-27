#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/global_wrapper.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockMakeLcAbs { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };

struct GlobalWrapperTest : public ::testing::Test {
    lc_expr_pool  lc;
    MockMakeLcAbs mock_abs;
    MockMakeLcApp mock_app;
    global_wrapper<MockMakeLcAbs, MockMakeLcApp> gw{mock_abs, mock_app};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
    }
};

} // namespace

TEST_F(GlobalWrapperTest, EmptyGlobalsReturnsMain) {
    const lc_expr* main = lc.make_var(0);
    EXPECT_EQ(gw.wrap(main, {}), main);
}

TEST_F(GlobalWrapperTest, SingleGlobalWrapsMain) {
    const lc_expr* main = lc.make_var(0);
    const lc_expr* g    = lc.make_abs(lc.make_var(0));
    // expect: app(abs(main), g)
    EXPECT_EQ(gw.wrap(main, {g}), lc.make_app(lc.make_abs(main), g));
}

TEST_F(GlobalWrapperTest, ThreeGlobalsInPushOrder) {
    // Globals pushed outermost-first: abc (idx 0 in vec), def (idx 1), ghi (idx 2).
    // Wrap iterates reversed: ghi innermost, then def, then abc outermost.
    // Result: app(abs(app(abs(app(abs(main), ghi)), def)), abc)
    const lc_expr* main = lc.make_var(99);
    const lc_expr* abc  = lc.make_var(0);
    const lc_expr* def_ = lc.make_var(1);
    const lc_expr* ghi  = lc.make_var(2);

    const lc_expr* step1 = lc.make_app(lc.make_abs(main), ghi);
    const lc_expr* step2 = lc.make_app(lc.make_abs(step1), def_);
    const lc_expr* step3 = lc.make_app(lc.make_abs(step2), abc);

    EXPECT_EQ(gw.wrap(main, {abc, def_, ghi}), step3);
}
