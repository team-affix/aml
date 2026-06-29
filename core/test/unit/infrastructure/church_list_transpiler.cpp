#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/church_list_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*)); };
struct MockMakeLcVar     { MOCK_METHOD(const lc_expr*, make_var,  (uint32_t)); };
struct MockMakeLcAbs     { MOCK_METHOD(const lc_expr*, make_abs,  (const lc_expr*)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app,  (const lc_expr*, const lc_expr*)); };

struct ChurchListTranspilerTest : public ::testing::Test {
    aml_expr_pool    aml;
    lc_expr_pool     lc;
    MockTranspileExpr mock_tx;
    MockMakeLcVar     mock_var;
    MockMakeLcAbs     mock_abs;
    MockMakeLcApp     mock_app;
    church_list_transpiler<MockTranspileExpr, MockMakeLcVar, MockMakeLcAbs, MockMakeLcApp>
        clt{mock_tx, mock_var, mock_abs, mock_app};

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

// Church list encoding: c_list = λf.λx. f e0 (f e1 (... (f e_{n-1} x)...))
// Using de Bruijn: var(0)=x (initial acc), var(1)=f (cons combinator)

TEST_F(ChurchListTranspilerTest, EmptyListIsLambdaFLambdaXX) {
    // [] → abs(abs(var(0)))
    EXPECT_EQ(clt.transpile_list(aml_expr::list{{}, {}}),
              ab(ab(v(0))));
}

TEST_F(ChurchListTranspilerTest, SingleElementList) {
    using testing::Return;
    const aml_expr* elem    = aml.make_token("a");
    const lc_expr*  elem_lc = lc.make_var(5);
    ON_CALL(mock_tx, transpile(elem)).WillByDefault(Return(elem_lc));

    // [a] → abs(abs(app(app(var(1), a_lc), var(0))))
    EXPECT_EQ(clt.transpile_list(aml_expr::list{{elem}, {}}),
              ab(ab(ap(ap(v(1), elem_lc), v(0)))));
}

TEST_F(ChurchListTranspilerTest, TwoElementsProduceNestedFold) {
    using testing::Return;
    const aml_expr* e0    = aml.make_token("a");
    const aml_expr* e1    = aml.make_token("b");
    const lc_expr*  lc0   = lc.make_var(5);
    const lc_expr*  lc1   = lc.make_var(6);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));

    // [e0, e1] → abs(abs(app(app(var(1),e0), app(app(var(1),e1), var(0)))))
    const lc_expr* inner = ap(ap(v(1), lc1), v(0));
    const lc_expr* body  = ap(ap(v(1), lc0), inner);
    EXPECT_EQ(clt.transpile_list(aml_expr::list{{e0, e1}, {}}), ab(ab(body)));
}

TEST_F(ChurchListTranspilerTest, AlwaysWrapsWithTwoAbstractions) {
    const lc_expr* result = clt.transpile_list(aml_expr::list{{}, {}});
    const auto* outer = std::get_if<lc_expr::abs>(&result->content);
    ASSERT_NE(outer, nullptr);
    const auto* inner = std::get_if<lc_expr::abs>(&outer->body->content);
    EXPECT_NE(inner, nullptr);
}

TEST_F(ChurchListTranspilerTest, EachElementTranspiledExactlyOnce) {
    const aml_expr* e0 = aml.make_token("x");
    const aml_expr* e1 = aml.make_token("y");

    EXPECT_CALL(mock_tx, transpile(e0)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e1)).Times(1);
    clt.transpile_list(aml_expr::list{{e0, e1}, {}});
}

TEST_F(ChurchListTranspilerTest, ThreeElementsProduceCorrectFold) {
    using testing::Return;
    const aml_expr* e0  = aml.make_token("a");
    const aml_expr* e1  = aml.make_token("b");
    const aml_expr* e2  = aml.make_token("c");
    const lc_expr*  lc0 = lc.make_var(5);
    const lc_expr*  lc1 = lc.make_var(6);
    const lc_expr*  lc2 = lc.make_var(7);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(lc2));

    // [e0, e1, e2]_church = abs(abs(f e0 (f e1 (f e2 x))))
    // f = var(1), x = var(0)
    const lc_expr* inner = ap(ap(v(1), lc2), v(0));
    const lc_expr* mid   = ap(ap(v(1), lc1), inner);
    const lc_expr* body  = ap(ap(v(1), lc0), mid);
    EXPECT_EQ(clt.transpile_list(aml_expr::list{{e0, e1, e2}, {}}), ab(ab(body)));
}
