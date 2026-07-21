#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <variant>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/list_transpiler.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/list_decl_group.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*)); };
struct MockMakeLcVar     { MOCK_METHOD(const lc_expr*, make_var,  (uint32_t)); };
struct MockMakeLcAbs     { MOCK_METHOD(const lc_expr*, make_abs,  (const lc_expr*)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app,  (const lc_expr*, const lc_expr*)); };
struct MockGetVarIndex   { MOCK_METHOD(uint32_t,       get_var_index, (const std::string&)); };

using NiceTx  = testing::NiceMock<MockTranspileExpr>;
using NiceVar = testing::NiceMock<MockMakeLcVar>;
using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;
using NiceIdx = testing::NiceMock<MockGetVarIndex>;

// Standard bindings: nil=0, cons=1
struct ListTranspilerTest : public ::testing::Test {
    aml_expr_pool aml;
    lc_expr_pool  lc;
    NiceTx  mock_tx;
    NiceVar mock_var;
    NiceAbs mock_abs;
    NiceApp mock_app;
    NiceIdx mock_idx;
    list_transpiler<NiceTx, NiceVar, NiceAbs, NiceApp, NiceIdx>
        lt{mock_tx, mock_var, mock_abs, mock_app, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_tx, transpile(_)).WillByDefault([this](const aml_expr* e) {
            return lc.make_var(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(e) & 0xFFFF));
        });
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index(k_nil_name)) .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index(k_cons_name)).WillByDefault(Return(1u));
    }

    const lc_expr* v(uint32_t i)                          { return lc.make_var(i); }
    const lc_expr* ab(const lc_expr* b)                   { return lc.make_abs(b); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(1), hd), tl);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Scott
// ---------------------------------------------------------------------------

TEST_F(ListTranspilerTest, ScottEmptyListReturnsNil) {
    EXPECT_EQ(lt.transpile_list({{}, list_format::scott}), v(0));
}

TEST_F(ListTranspilerTest, ScottSingleElementIsConsElemNil) {
    using testing::Return;
    const aml_expr* elem    = aml.make_symbol("a");
    const lc_expr*  elem_lc = lc.make_var(10);
    ON_CALL(mock_tx, transpile(elem)).WillByDefault(Return(elem_lc));

    EXPECT_EQ(lt.transpile_list({{elem}, list_format::scott}),
              cons_(elem_lc, v(0)));
}

TEST_F(ListTranspilerTest, ScottTwoElementsInOrder) {
    using testing::Return;
    const aml_expr* e0  = aml.make_symbol("a");
    const aml_expr* e1  = aml.make_symbol("b");
    const lc_expr*  lc0 = lc.make_var(10);
    const lc_expr*  lc1 = lc.make_var(11);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));

    EXPECT_EQ(lt.transpile_list({{e0, e1}, list_format::scott}),
              cons_(lc0, cons_(lc1, v(0))));
}

TEST_F(ListTranspilerTest, ScottEachElementTranspiledThroughTranspileExpr) {
    const aml_expr* e0 = aml.make_symbol("x");
    const aml_expr* e1 = aml.make_symbol("y");
    const aml_expr* e2 = aml.make_symbol("z");

    EXPECT_CALL(mock_tx, transpile(e0)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e1)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e2)).Times(1);
    lt.transpile_list({{e0, e1, e2}, list_format::scott});
}

TEST_F(ListTranspilerTest, ScottThreeElementsInOrder) {
    using testing::Return;
    const aml_expr* e0  = aml.make_symbol("a");
    const aml_expr* e1  = aml.make_symbol("b");
    const aml_expr* e2  = aml.make_symbol("c");
    const lc_expr*  lc0 = lc.make_var(10);
    const lc_expr*  lc1 = lc.make_var(11);
    const lc_expr*  lc2 = lc.make_var(12);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(lc2));

    EXPECT_EQ(lt.transpile_list({{e0, e1, e2}, list_format::scott}),
              cons_(lc0, cons_(lc1, cons_(lc2, v(0)))));
}

// ---------------------------------------------------------------------------
// Church
// ---------------------------------------------------------------------------

TEST_F(ListTranspilerTest, ChurchEmptyListIsLambdaFLambdaXX) {
    EXPECT_EQ(lt.transpile_list({{}, list_format::church}),
              ab(ab(v(0))));
}

TEST_F(ListTranspilerTest, ChurchSingleElementList) {
    using testing::Return;
    const aml_expr* elem    = aml.make_symbol("a");
    const lc_expr*  elem_lc = lc.make_var(5);
    ON_CALL(mock_tx, transpile(elem)).WillByDefault(Return(elem_lc));

    EXPECT_EQ(lt.transpile_list({{elem}, list_format::church}),
              ab(ab(ap(ap(v(1), elem_lc), v(0)))));
}

TEST_F(ListTranspilerTest, ChurchTwoElementsProduceNestedFold) {
    using testing::Return;
    const aml_expr* e0  = aml.make_symbol("a");
    const aml_expr* e1  = aml.make_symbol("b");
    const lc_expr*  lc0 = lc.make_var(5);
    const lc_expr*  lc1 = lc.make_var(6);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));

    const lc_expr* inner = ap(ap(v(1), lc1), v(0));
    const lc_expr* body  = ap(ap(v(1), lc0), inner);
    EXPECT_EQ(lt.transpile_list({{e0, e1}, list_format::church}), ab(ab(body)));
}

TEST_F(ListTranspilerTest, ChurchAlwaysWrapsWithTwoAbstractions) {
    const lc_expr* result = lt.transpile_list({{}, list_format::church});
    const auto* outer = std::get_if<lc_expr::abs>(&result->content);
    ASSERT_NE(outer, nullptr);
    const auto* inner = std::get_if<lc_expr::abs>(&outer->body->content);
    EXPECT_NE(inner, nullptr);
}

TEST_F(ListTranspilerTest, ChurchEachElementTranspiledExactlyOnce) {
    const aml_expr* e0 = aml.make_symbol("x");
    const aml_expr* e1 = aml.make_symbol("y");

    EXPECT_CALL(mock_tx, transpile(e0)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e1)).Times(1);
    lt.transpile_list({{e0, e1}, list_format::church});
}

TEST_F(ListTranspilerTest, ChurchThreeElementsProduceCorrectFold) {
    using testing::Return;
    const aml_expr* e0  = aml.make_symbol("a");
    const aml_expr* e1  = aml.make_symbol("b");
    const aml_expr* e2  = aml.make_symbol("c");
    const lc_expr*  lc0 = lc.make_var(5);
    const lc_expr*  lc1 = lc.make_var(6);
    const lc_expr*  lc2 = lc.make_var(7);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(lc2));

    const lc_expr* inner = ap(ap(v(1), lc2), v(0));
    const lc_expr* mid   = ap(ap(v(1), lc1), inner);
    const lc_expr* body  = ap(ap(v(1), lc0), mid);
    EXPECT_EQ(lt.transpile_list({{e0, e1, e2}, list_format::church}), ab(ab(body)));
}
