#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/abs_transpiler.hpp"
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"

namespace {

struct MockTranspileExpr { MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*)); };
struct MockMakeLcAbs     { MOCK_METHOD(const lc_expr*, make_abs,  (const lc_expr*)); };
struct MockPushVar       { MOCK_METHOD(void,            push,     (std::string)); };
struct MockPopVar        { MOCK_METHOD(void,            pop,      ()); };

struct AbsTranspilerTest : public ::testing::Test {
    aml_expr_pool aml;
    lc_expr_pool  lc;

    MockTranspileExpr mock_tx;
    MockMakeLcAbs     mock_abs;
    MockPushVar       mock_push;
    MockPopVar        mock_pop;

    abs_transpiler<MockTranspileExpr, MockMakeLcAbs, MockPushVar, MockPopVar>
        at{mock_tx, mock_abs, mock_push, mock_pop};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_tx, transpile(_)).WillByDefault([this](const aml_expr*) {
            return lc.make_var(0);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
    }
};

} // namespace

// The observable contract:
//  1. push(param) is called once (scope mutation — required).
//  2. transpile(body) is called once to obtain the body lc_expr.
//  3. make_abs(body_result) is called once to wrap it.
//  4. pop() is called once to undo the push (scope hygiene — required).
//  5. The returned value is the result of make_abs.

TEST_F(AbsTranspilerTest, PushesParamBeforeTranspile) {
    using testing::InSequence;
    using testing::_;

    const aml_expr* body_ptr = aml.make_token("x");
    const lc_expr*  body_lc  = lc.make_var(0);

    ON_CALL(mock_tx,  transpile(_)).WillByDefault(testing::Return(body_lc));
    ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
        return lc.make_abs(b);
    });

    {
        InSequence seq;
        EXPECT_CALL(mock_push, push(std::string("x"))).Times(1);
        EXPECT_CALL(mock_tx,   transpile(body_ptr)).Times(1);
        EXPECT_CALL(mock_abs,  make_abs(body_lc)).Times(1);
        EXPECT_CALL(mock_pop,  pop()).Times(1);
    }

    at.transpile_abs(aml_expr::abs{"x", body_ptr});
}

TEST_F(AbsTranspilerTest, ReturnsResultOfMakeAbs) {
    using testing::Return;
    using testing::_;

    const aml_expr* body_ptr = aml.make_token("y");
    const lc_expr*  body_lc  = lc.make_var(0);
    const lc_expr*  abs_lc   = lc.make_abs(body_lc);

    ON_CALL(mock_tx,  transpile(_)).WillByDefault(Return(body_lc));
    ON_CALL(mock_abs, make_abs(_)).WillByDefault(Return(abs_lc));
    ON_CALL(mock_push, push(_)).WillByDefault(testing::Return());
    ON_CALL(mock_pop,  pop()).WillByDefault(testing::Return());

    EXPECT_EQ(at.transpile_abs(aml_expr::abs{"y", body_ptr}), abs_lc);
}

TEST_F(AbsTranspilerTest, PopCalledEvenWhenBodyIsSimple) {
    using testing::_;
    EXPECT_CALL(mock_pop, pop()).Times(1);
    at.transpile_abs(aml_expr::abs{"z", aml.make_token("z")});
}

// ---------------------------------------------------------------------------
// Multi-layer nesting: λx.(λy.body) — the LIFO push/pop invariant
// ---------------------------------------------------------------------------

// When the body of an abs is itself an abs, transpile_abs is called
// recursively (simulated here by the mock calling back into at).
// The scope contract requires: outer push → inner push → inner pop → outer pop.

TEST_F(AbsTranspilerTest, NestedAbsHasProperlySequencedPushPop) {
    using testing::InSequence;
    using testing::_;

    const aml_expr* inner_leaf = aml.make_token("inner_leaf");
    const aml_expr* outer_body = aml.make_abs("y", inner_leaf);

    // Simulate the real dispatcher: when transpile sees an abs node, it
    // calls transpile_abs recursively.
    ON_CALL(mock_tx, transpile(outer_body))
        .WillByDefault([this](const aml_expr* e) -> const lc_expr* {
            return at.transpile_abs(std::get<aml_expr::abs>(e->content));
        });
    // inner_leaf is a leaf — return a stable sentinel.
    ON_CALL(mock_tx, transpile(inner_leaf))
        .WillByDefault(testing::Return(lc.make_var(42)));

    // gmock records calls synchronously in the order they are made.
    // push("y") fires inside the transpile(outer_body) action, so it arrives
    // between push("x") and the outer pop().
    {
        InSequence seq;
        EXPECT_CALL(mock_push, push(std::string("x")));  // (1) outer push
        EXPECT_CALL(mock_push, push(std::string("y")));  // (2) inner push
        EXPECT_CALL(mock_pop,  pop());                   // (3) inner pop
        EXPECT_CALL(mock_pop,  pop());                   // (4) outer pop
    }

    at.transpile_abs(aml_expr::abs{"x", outer_body});
}

TEST_F(AbsTranspilerTest, NestedAbsReturnsCorrectStructure) {
    using testing::_;

    const aml_expr* inner_leaf = aml.make_token("inner_leaf");
    const aml_expr* outer_body = aml.make_abs("y", inner_leaf);

    ON_CALL(mock_tx, transpile(outer_body))
        .WillByDefault([this](const aml_expr* e) -> const lc_expr* {
            return at.transpile_abs(std::get<aml_expr::abs>(e->content));
        });
    ON_CALL(mock_tx, transpile(inner_leaf))
        .WillByDefault(testing::Return(lc.make_var(42)));

    // make_abs(make_abs(var(42))): outermost wraps the inner result.
    const lc_expr* expected = lc.make_abs(lc.make_abs(lc.make_var(42)));
    EXPECT_EQ(at.transpile_abs(aml_expr::abs{"x", outer_body}), expected);
}
