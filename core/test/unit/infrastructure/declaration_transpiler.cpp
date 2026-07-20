#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/declaration_transpiler.hpp"
#include "value_objects/lc_expr.hpp"

namespace {

struct MockMakeLcVar {
    MOCK_METHOD(const lc_expr*, make_var, (uint32_t), ());
};
struct MockMakeLcAbs {
    MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*), ());
};
struct MockMakeLcApp {
    MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*), ());
};

using NiceVar = testing::NiceMock<MockMakeLcVar>;
using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;

struct DeclarationTranspilerTest : public ::testing::Test {
    NiceVar mock_var;
    NiceAbs mock_abs;
    NiceApp mock_app;
    declaration_transpiler<NiceVar, NiceAbs, NiceApp> dt{mock_var, mock_abs, mock_app};
    lc_expr v0{};
    lc_expr v1{};
    lc_expr v2{};
    lc_expr v3{};
    lc_expr app0{};
    lc_expr app1{};
    lc_expr app2{};
    lc_expr abs0{};
    lc_expr abs1{};
    lc_expr abs2{};
    lc_expr abs3{};
};

} // namespace

TEST_F(DeclarationTranspilerTest, SingleVariantArity0) {
    using testing::Return;
    using testing::InSequence;

    // n=1, k=0, a=0: abs(var(0))
    {
        InSequence seq;
        EXPECT_CALL(mock_var, make_var(0u)).WillOnce(Return(&v0));
        EXPECT_CALL(mock_abs, make_abs(&v0)).WillOnce(Return(&abs0));
    }
    EXPECT_CALL(mock_app, make_app(testing::_, testing::_)).Times(0);
    EXPECT_EQ(dt.transpile_decl(1u, 0u, 0u), &abs0);
}

TEST_F(DeclarationTranspilerTest, BooleanTrueEncoding) {
    using testing::Return;
    using testing::InSequence;

    // n=2, k=0, a=0: abs(abs(var(1)))
    {
        InSequence seq;
        EXPECT_CALL(mock_var, make_var(1u)).WillOnce(Return(&v1));
        EXPECT_CALL(mock_abs, make_abs(&v1)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_abs, make_abs(&abs0)).WillOnce(Return(&abs1));
    }
    EXPECT_EQ(dt.transpile_decl(2u, 0u, 0u), &abs1);
}

TEST_F(DeclarationTranspilerTest, BooleanFalseEncoding) {
    using testing::Return;
    using testing::InSequence;

    // n=2, k=1, a=0: abs(abs(var(0)))
    {
        InSequence seq;
        EXPECT_CALL(mock_var, make_var(0u)).WillOnce(Return(&v0));
        EXPECT_CALL(mock_abs, make_abs(&v0)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_abs, make_abs(&abs0)).WillOnce(Return(&abs1));
    }
    EXPECT_EQ(dt.transpile_decl(2u, 1u, 0u), &abs1);
}

TEST_F(DeclarationTranspilerTest, ArityOneAppliesThenWraps) {
    using testing::Return;
    using testing::InSequence;

    // n=3, k=2, a=1: abs^4(app(var(1), var(0)))
    {
        InSequence seq;
        EXPECT_CALL(mock_var, make_var(1u)).WillOnce(Return(&v1));
        EXPECT_CALL(mock_var, make_var(0u)).WillOnce(Return(&v0));
        EXPECT_CALL(mock_app, make_app(&v1, &v0)).WillOnce(Return(&app0));
        EXPECT_CALL(mock_abs, make_abs(&app0)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_abs, make_abs(&abs0)).WillOnce(Return(&abs1));
        EXPECT_CALL(mock_abs, make_abs(&abs1)).WillOnce(Return(&abs2));
        EXPECT_CALL(mock_abs, make_abs(&abs2)).WillOnce(Return(&abs3));
    }
    EXPECT_EQ(dt.transpile_decl(3u, 2u, 1u), &abs3);
}

TEST_F(DeclarationTranspilerTest, HighAritySingleVariant) {
    using testing::Return;
    using testing::InSequence;

    // n=1, k=0, a=3: abs^4(app(app(app(var(3), var(2)), var(1)), var(0)))
    {
        InSequence seq;
        EXPECT_CALL(mock_var, make_var(3u)).WillOnce(Return(&v3));
        EXPECT_CALL(mock_var, make_var(2u)).WillOnce(Return(&v2));
        EXPECT_CALL(mock_app, make_app(&v3, &v2)).WillOnce(Return(&app0));
        EXPECT_CALL(mock_var, make_var(1u)).WillOnce(Return(&v1));
        EXPECT_CALL(mock_app, make_app(&app0, &v1)).WillOnce(Return(&app1));
        EXPECT_CALL(mock_var, make_var(0u)).WillOnce(Return(&v0));
        EXPECT_CALL(mock_app, make_app(&app1, &v0)).WillOnce(Return(&app2));
        EXPECT_CALL(mock_abs, make_abs(&app2)).WillOnce(Return(&abs0));
        EXPECT_CALL(mock_abs, make_abs(&abs0)).WillOnce(Return(&abs1));
        EXPECT_CALL(mock_abs, make_abs(&abs1)).WillOnce(Return(&abs2));
        EXPECT_CALL(mock_abs, make_abs(&abs2)).WillOnce(Return(&abs3));
    }
    EXPECT_EQ(dt.transpile_decl(1u, 0u, 3u), &abs3);
}
