#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockMakeLcAbs { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };

using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;
using GlobalStack = std::stack<const lc_expr*>;

struct AssemblerTest : public ::testing::Test {
    lc_expr_pool lc;
    NiceAbs      mock_abs;
    NiceApp      mock_app;
    GlobalStack  globals;
    assembler<NiceAbs, NiceApp, GlobalStack> asm_{mock_abs, mock_app, globals};

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
    // Expected: app(abs(nullptr), g)
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
// pointer or a leftover value.
TEST_F(AssemblerTest, NullptrIsAlwaysTheInnermostBody) {
    using testing::InSequence;

    const lc_expr* g = lc.make_var(0);
    globals.push(g);
    {
        InSequence seq;
        EXPECT_CALL(mock_abs, make_abs(nullptr)).Times(1);
        EXPECT_CALL(mock_app, make_app(testing::_, g)).Times(1);
    }
    asm_.assemble();
}
