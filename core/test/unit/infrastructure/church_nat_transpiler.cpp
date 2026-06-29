#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/church_nat_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockMakeLcVar { MOCK_METHOD(const lc_expr*, make_var, (uint32_t)); };
struct MockMakeLcAbs { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };

struct ChurchNatTranspilerTest : public ::testing::Test {
    lc_expr_pool  lc;
    MockMakeLcVar mock_var;
    MockMakeLcAbs mock_abs;
    MockMakeLcApp mock_app;
    church_nat_transpiler<MockMakeLcVar, MockMakeLcAbs, MockMakeLcApp>
        cnt{mock_var, mock_abs, mock_app};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ab(const lc_expr* b)                    { return lc.make_abs(b); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
};

} // namespace

// Church encoding: c_n = λf.λx. f^n(x)
// Using de Bruijn indices: var(0)=x, var(1)=f

TEST_F(ChurchNatTranspilerTest, ZeroIsLambdaFLambdaXX) {
    // c_0 = λf.λx.x = abs(abs(var(0)))
    EXPECT_EQ(cnt.transpile_nat({0u, nat_format::church}), ab(ab(v(0))));
}

TEST_F(ChurchNatTranspilerTest, OneIsLambdaFLambdaXFX) {
    // c_1 = λf.λx. f x = abs(abs(app(var(1), var(0))))
    EXPECT_EQ(cnt.transpile_nat({1u, nat_format::church}),
              ab(ab(ap(v(1), v(0)))));
}

TEST_F(ChurchNatTranspilerTest, TwoIsLambdaFLambdaXFFX) {
    // c_2 = λf.λx. f(f x) = abs(abs(app(var(1), app(var(1), var(0)))))
    EXPECT_EQ(cnt.transpile_nat({2u, nat_format::church}),
              ab(ab(ap(v(1), ap(v(1), v(0))))));
}

TEST_F(ChurchNatTranspilerTest, ThreeAppliesFThreeTimes) {
    // c_3 = λf.λx. f(f(f x))
    EXPECT_EQ(cnt.transpile_nat({3u, nat_format::church}),
              ab(ab(ap(v(1), ap(v(1), ap(v(1), v(0)))))));
}

TEST_F(ChurchNatTranspilerTest, AlwaysWrapsWithTwoAbstractions) {
    // Every church numeral has exactly 2 outer lambdas
    const lc_expr* c4 = cnt.transpile_nat({4u, nat_format::church});
    const auto* outer = std::get_if<lc_expr::abs>(&c4->content);
    ASSERT_NE(outer, nullptr);
    const auto* inner = std::get_if<lc_expr::abs>(&outer->body->content);
    EXPECT_NE(inner, nullptr);
}
