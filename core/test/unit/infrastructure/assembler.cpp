#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockMakeLcAbs { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };
struct MockTopLcExpr { MOCK_METHOD(const lc_expr*, top, (), (const)); };
struct MockPopLcExpr {
    MOCK_METHOD(bool, empty, (), (const));
    MOCK_METHOD(void, pop,   ());
};

using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;
using GlobalStack = std::stack<const lc_expr*>;

// Fixture uses a real stack for both ITopLcExpr and IPopLcExpr.  Tests that
// need to verify the call contract on top/pop/empty can construct a local
// assembler with MockTopLcExpr / MockPopLcExpr instead.
struct AssemblerTest : public ::testing::Test {
    lc_expr_pool  lc;
    NiceAbs       mock_abs;
    NiceApp       mock_app;
    GlobalStack   globals;
    assembler<NiceAbs, NiceApp, GlobalStack, GlobalStack> asm_{
        mock_abs, mock_app, globals, globals};

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

// Empty stack: loop does not execute; nullptr is returned directly.
TEST_F(AssemblerTest, EmptyGlobalsReturnsNullptr) {
    EXPECT_CALL(mock_abs, make_abs(testing::_)).Times(0);
    EXPECT_CALL(mock_app, make_app(testing::_, testing::_)).Times(0);
    EXPECT_EQ(asm_.assemble(), nullptr);
}

// Single global: make_abs receives nullptr (the innermost body), then make_app
// wraps it with the one global.
TEST_F(AssemblerTest, SingleGlobalWrapsNullptr) {
    const lc_expr* g = lc.make_var(0);
    globals.push(g);
    EXPECT_EQ(asm_.assemble(), lc.make_app(lc.make_abs(nullptr), g));
}

// Three globals pushed in definition order (g0 first = bottom, g2 last = top).
// LIFO popping places g2 innermost and g0 outermost.
// A bug that changed the popping order would produce g0 innermost instead.
TEST_F(AssemblerTest, ThreeGlobalsFirstDefinedIsOutermost) {
    const lc_expr* g0 = lc.make_var(0);
    const lc_expr* g1 = lc.make_var(1);
    const lc_expr* g2 = lc.make_var(2);
    globals.push(g0);
    globals.push(g1);
    globals.push(g2);

    const lc_expr* step1 = lc.make_app(lc.make_abs(nullptr), g2);
    const lc_expr* step2 = lc.make_app(lc.make_abs(step1),   g1);
    const lc_expr* step3 = lc.make_app(lc.make_abs(step2),   g0);

    EXPECT_EQ(asm_.assemble(), step3);
}

// The very first make_abs call must receive nullptr — not some uninitialized
// pointer or a leftover value.  Uses separate mocks for top/pop so each
// interface can be verified independently of the real stack implementation.
//
// Note: make_abs(nullptr) and top() are both arguments to make_app, so their
// evaluation order is unspecified by C++.  We assert them independently
// without InSequence.  The loop-level order (empty → body → pop → empty) is
// asserted via WillOnce chaining on mock_pop.
TEST(AssemblerTest_NullptrInnermost, NullptrIsAlwaysTheInnermostBody) {
    using testing::Return;

    lc_expr_pool lc;
    const lc_expr* g       = lc.make_var(0);
    const lc_expr* abs_nil = lc.make_abs(nullptr);
    const lc_expr* result  = lc.make_app(abs_nil, g);

    testing::NiceMock<MockMakeLcAbs> mock_abs;
    testing::NiceMock<MockMakeLcApp> mock_app;
    MockTopLcExpr mock_top;
    MockPopLcExpr mock_pop;

    ON_CALL(mock_abs, make_abs(nullptr)).WillByDefault(Return(abs_nil));
    ON_CALL(mock_app, make_app(abs_nil, g)).WillByDefault(Return(result));
    ON_CALL(mock_top, top()).WillByDefault(Return(g));

    assembler<testing::NiceMock<MockMakeLcAbs>,
              testing::NiceMock<MockMakeLcApp>,
              MockTopLcExpr, MockPopLcExpr> asm_{mock_abs, mock_app, mock_top, mock_pop};

    // empty() drives the loop: one iteration then stop.
    EXPECT_CALL(mock_pop, empty())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    // The key contract: make_abs receives nullptr as the seed value.
    EXPECT_CALL(mock_abs, make_abs(nullptr)).Times(1);
    EXPECT_CALL(mock_top, top()).Times(testing::AtLeast(1));
    EXPECT_CALL(mock_pop, pop()).Times(1);

    EXPECT_EQ(asm_.assemble(), result);
}
