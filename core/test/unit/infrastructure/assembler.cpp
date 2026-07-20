#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/assembler.hpp"
#include "value_objects/lc_expr.hpp"

namespace {

struct MockMakeLcAbs {
    MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*), ());
};
struct MockMakeLcApp {
    MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*), ());
};
struct MockGlobalsStack {
    MOCK_METHOD(bool, empty, (), (const));
    MOCK_METHOD(const lc_expr*, pop, (), ());
};

using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;
using NiceGl  = testing::NiceMock<MockGlobalsStack>;

struct AssemblerTest : public ::testing::Test {
    NiceAbs mock_abs;
    NiceApp mock_app;
    NiceGl  mock_globals;
    assembler<NiceAbs, NiceApp, NiceGl> asm_{mock_abs, mock_app, mock_globals};
    lc_expr body{};
    lc_expr g0{};
    lc_expr g1{};
    lc_expr g2{};
    lc_expr abs0{};
    lc_expr abs1{};
    lc_expr abs2{};
    lc_expr app0{};
    lc_expr app1{};
    lc_expr app2{};
};

} // namespace

TEST_F(AssemblerTest, EmptyGlobalsReturnsBody) {
    using testing::Return;

    EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    EXPECT_CALL(mock_abs, make_abs(testing::_)).Times(0);
    EXPECT_CALL(mock_app, make_app(testing::_, testing::_)).Times(0);
    EXPECT_EQ(asm_.assemble(&body), &body);
}

TEST_F(AssemblerTest, EmptyGlobalsNullptrBody) {
    using testing::Return;

    EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    EXPECT_EQ(asm_.assemble(nullptr), nullptr);
}

TEST_F(AssemblerTest, SingleGlobalWrapsBody) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(false));
        EXPECT_CALL(mock_globals, pop()).WillOnce(Return(&g0));
        EXPECT_CALL(mock_abs, make_abs(&body)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_app, make_app(&abs0, &g0)).WillOnce(Return(&app0));
        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    }
    EXPECT_EQ(asm_.assemble(&body), &app0);
}

TEST_F(AssemblerTest, ThreeGlobalsFirstDefinedIsOutermost) {
    using testing::Return;
    using testing::InSequence;

    // LIFO drain: last-defined (g2) first, first-defined (g0) outermost
    {
        InSequence seq;
        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(false));
        EXPECT_CALL(mock_globals, pop()).WillOnce(Return(&g2));
        EXPECT_CALL(mock_abs, make_abs(&body)).WillOnce(Return(&abs2));
        EXPECT_CALL(mock_app, make_app(&abs2, &g2)).WillOnce(Return(&app2));

        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(false));
        EXPECT_CALL(mock_globals, pop()).WillOnce(Return(&g1));
        EXPECT_CALL(mock_abs, make_abs(&app2)).WillOnce(Return(&abs1));
        EXPECT_CALL(mock_app, make_app(&abs1, &g1)).WillOnce(Return(&app1));

        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(false));
        EXPECT_CALL(mock_globals, pop()).WillOnce(Return(&g0));
        EXPECT_CALL(mock_abs, make_abs(&app1)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_app, make_app(&abs0, &g0)).WillOnce(Return(&app0));

        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    }
    EXPECT_EQ(asm_.assemble(&body), &app0);
}

TEST_F(AssemblerTest, AssembleDrainsStack) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(false));
        EXPECT_CALL(mock_globals, pop()).WillOnce(Return(&g0));
        EXPECT_CALL(mock_abs, make_abs(&body)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_app, make_app(&abs0, &g0)).WillOnce(Return(&app0));
        EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    }
    EXPECT_EQ(asm_.assemble(&body), &app0);

    EXPECT_CALL(mock_globals, empty()).WillOnce(Return(true));
    EXPECT_EQ(asm_.assemble(&g1), &g1);
}
