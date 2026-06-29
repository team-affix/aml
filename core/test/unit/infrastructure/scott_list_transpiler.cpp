#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scott_list_transpiler.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile,      (const aml_expr*)); };
struct MockMakeLcVar     { MOCK_METHOD(const lc_expr*, make_var,        (uint32_t)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app,        (const lc_expr*, const lc_expr*)); };
struct MockGetVarIndex   { MOCK_METHOD(uint32_t,       get_var_index,   (const std::string&)); };

// Standard bindings: nil=0, cons=1
struct ScottListTranspilerTest : public ::testing::Test {
    aml_expr_pool    aml;
    lc_expr_pool     lc;
    MockTranspileExpr mock_tx;
    MockMakeLcVar     mock_var;
    MockMakeLcApp     mock_app;
    MockGetVarIndex   mock_idx;
    scott_list_transpiler<MockTranspileExpr, MockMakeLcVar, MockMakeLcApp, MockGetVarIndex>
        slt{mock_tx, mock_var, mock_app, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_tx, transpile(_)).WillByDefault([this](const aml_expr* e) {
            // Use pointer address as a sentinel index so each element is distinct
            return lc.make_var(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(e) & 0xFFFF));
        });
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index("nil")) .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index("cons")).WillByDefault(Return(1u));
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(1), hd), tl);
    }
};

} // namespace

TEST_F(ScottListTranspilerTest, EmptyListReturnsNil) {
    EXPECT_EQ(slt.transpile_list(aml_expr::list{{}, {}}), v(0));
}

TEST_F(ScottListTranspilerTest, SingleElementIsConsElemNil) {
    using testing::Return;
    const aml_expr* elem    = aml.make_token("a");
    const lc_expr*  elem_lc = lc.make_var(10);
    ON_CALL(mock_tx, transpile(elem)).WillByDefault(Return(elem_lc));

    EXPECT_EQ(slt.transpile_list(aml_expr::list{{elem}, {}}),
              cons_(elem_lc, v(0)));
}

TEST_F(ScottListTranspilerTest, TwoElementsInOrder) {
    using testing::Return;
    const aml_expr* e0    = aml.make_token("a");
    const aml_expr* e1    = aml.make_token("b");
    const lc_expr*  lc0   = lc.make_var(10);
    const lc_expr*  lc1   = lc.make_var(11);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));

    // [e0, e1] → cons(e0, cons(e1, nil))
    EXPECT_EQ(slt.transpile_list(aml_expr::list{{e0, e1}, {}}),
              cons_(lc0, cons_(lc1, v(0))));
}

TEST_F(ScottListTranspilerTest, EachElementTranspiledThroughTranspileExpr) {
    const aml_expr* e0 = aml.make_token("x");
    const aml_expr* e1 = aml.make_token("y");
    const aml_expr* e2 = aml.make_token("z");

    EXPECT_CALL(mock_tx, transpile(e0)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e1)).Times(1);
    EXPECT_CALL(mock_tx, transpile(e2)).Times(1);
    slt.transpile_list(aml_expr::list{{e0, e1, e2}, {}});
}

TEST_F(ScottListTranspilerTest, ThreeElementsInOrder) {
    using testing::Return;
    const aml_expr* e0  = aml.make_token("a");
    const aml_expr* e1  = aml.make_token("b");
    const aml_expr* e2  = aml.make_token("c");
    const lc_expr*  lc0 = lc.make_var(10);
    const lc_expr*  lc1 = lc.make_var(11);
    const lc_expr*  lc2 = lc.make_var(12);
    ON_CALL(mock_tx, transpile(e0)).WillByDefault(Return(lc0));
    ON_CALL(mock_tx, transpile(e1)).WillByDefault(Return(lc1));
    ON_CALL(mock_tx, transpile(e2)).WillByDefault(Return(lc2));

    // [e0, e1, e2] → cons(e0, cons(e1, cons(e2, nil)))
    EXPECT_EQ(slt.transpile_list(aml_expr::list{{e0, e1, e2}, {}}),
              cons_(lc0, cons_(lc1, cons_(lc2, v(0)))));
}
