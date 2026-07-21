#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <variant>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/list_transpiler.hpp"
#include "value_objects/list_format.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*)); };
struct MockMakeLcVar     { MOCK_METHOD(const lc_expr*, make_var,  (uint32_t)); };
struct MockMakeLcAbs     { MOCK_METHOD(const lc_expr*, make_abs,  (const lc_expr*)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app,  (const lc_expr*, const lc_expr*)); };
struct MockTranspileNil  { MOCK_METHOD(const lc_expr*, transpile_nil, ()); };
struct MockTranspileCons { MOCK_METHOD(const lc_expr*, transpile_cons, ()); };

using NiceTx   = testing::NiceMock<MockTranspileExpr>;
using NiceVar  = testing::NiceMock<MockMakeLcVar>;
using NiceAbs  = testing::NiceMock<MockMakeLcAbs>;
using NiceApp  = testing::NiceMock<MockMakeLcApp>;
using NiceNil  = testing::NiceMock<MockTranspileNil>;
using NiceCons = testing::NiceMock<MockTranspileCons>;

// Standard bindings: nil → var(0), cons → var(1)
struct ListTranspilerTest : public ::testing::Test {
    aml_expr_pool aml;
    lc_expr_pool  lc;
    NiceTx   mock_tx;
    NiceVar  mock_var;
    NiceAbs  mock_abs;
    NiceApp  mock_app;
    NiceNil  mock_nil;
    NiceCons mock_cons;
    list_transpiler<NiceTx, NiceVar, NiceAbs, NiceApp, NiceNil, NiceCons>
        lt{mock_tx, mock_var, mock_abs, mock_app, mock_nil, mock_cons};

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
        ON_CALL(mock_nil, transpile_nil()).WillByDefault(Return(lc.make_var(0)));
        ON_CALL(mock_cons, transpile_cons()).WillByDefault(Return(lc.make_var(1)));
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

TEST_F(ListTranspilerTest, ScottTwoElementsNestCons) {
    using testing::Return;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    const lc_expr*  l0 = lc.make_var(10);
    const lc_expr*  l1 = lc.make_var(11);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(l0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(l1));

    EXPECT_EQ(lt.transpile_list({{e0, e1}, list_format::scott}),
              cons_(l0, cons_(l1, v(0))));
}

TEST_F(ListTranspilerTest, ScottThreeElementsNestCons) {
    using testing::Return;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    const aml_expr* e2 = aml.make_symbol("c");
    const lc_expr*  l0 = lc.make_var(10);
    const lc_expr*  l1 = lc.make_var(11);
    const lc_expr*  l2 = lc.make_var(12);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(l0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(l1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(l2));

    EXPECT_EQ(lt.transpile_list({{e0, e1, e2}, list_format::scott}),
              cons_(l0, cons_(l1, cons_(l2, v(0)))));
}

TEST_F(ListTranspilerTest, ScottEachElementTranspiledExactlyOnce) {
    using testing::Return;
    using testing::Exactly;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc.make_var(10)));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc.make_var(11)));

    EXPECT_CALL(mock_tx, transpile(e0)).Times(Exactly(1));
    EXPECT_CALL(mock_tx, transpile(e1)).Times(Exactly(1));
    lt.transpile_list({{e0, e1}, list_format::scott});
}

// ---------------------------------------------------------------------------
// Church
// ---------------------------------------------------------------------------

TEST_F(ListTranspilerTest, ChurchEmptyListIsAbsAbsVar0) {
    EXPECT_EQ(lt.transpile_list({{}, list_format::church}),
              ab(ab(v(0))));
}

TEST_F(ListTranspilerTest, ChurchSingleElementList) {
    using testing::Return;
    const aml_expr* elem    = aml.make_symbol("a");
    const lc_expr*  elem_lc = lc.make_var(10);
    ON_CALL(mock_tx, transpile(elem)).WillByDefault(Return(elem_lc));

    // abs(abs(app(app(var(1), elem), var(0))))
    EXPECT_EQ(lt.transpile_list({{elem}, list_format::church}),
              ab(ab(ap(ap(v(1), elem_lc), v(0)))));
}

TEST_F(ListTranspilerTest, ChurchTwoElementsProduceNestedFold) {
    using testing::Return;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    const lc_expr*  l0 = lc.make_var(10);
    const lc_expr*  l1 = lc.make_var(11);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(l0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(l1));

    // fold right: app(app(c, e0), app(app(c, e1), n))
    const lc_expr* body = ap(ap(v(1), l0), ap(ap(v(1), l1), v(0)));
    EXPECT_EQ(lt.transpile_list({{e0, e1}, list_format::church}), ab(ab(body)));
}

TEST_F(ListTranspilerTest, ChurchAlwaysWrapsWithTwoAbstractions) {
    const lc_expr* result = lt.transpile_list({{}, list_format::church});
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(result->content));
    const lc_expr* inner = std::get<lc_expr::abs>(result->content).body;
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(inner->content));
}

TEST_F(ListTranspilerTest, ChurchEachElementTranspiledExactlyOnce) {
    using testing::Return;
    using testing::Exactly;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc.make_var(10)));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc.make_var(11)));

    EXPECT_CALL(mock_tx, transpile(e0)).Times(Exactly(1));
    EXPECT_CALL(mock_tx, transpile(e1)).Times(Exactly(1));
    lt.transpile_list({{e0, e1}, list_format::church});
}

TEST_F(ListTranspilerTest, ChurchThreeElementsProduceCorrectFold) {
    using testing::Return;
    const aml_expr* e0 = aml.make_symbol("a");
    const aml_expr* e1 = aml.make_symbol("b");
    const aml_expr* e2 = aml.make_symbol("c");
    const lc_expr*  l0 = lc.make_var(10);
    const lc_expr*  l1 = lc.make_var(11);
    const lc_expr*  l2 = lc.make_var(12);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(l0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(l1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(l2));

    const lc_expr* body =
        ap(ap(v(1), l0), ap(ap(v(1), l1), ap(ap(v(1), l2), v(0))));
    EXPECT_EQ(lt.transpile_list({{e0, e1, e2}, list_format::church}), ab(ab(body)));
}
