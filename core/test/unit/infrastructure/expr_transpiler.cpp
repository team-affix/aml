// Transpiler unit tests: aml_expr -> lc_expr with pure LC name resolution.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/list_format.hpp"

namespace {

static const std::vector<std::string> kBuiltinNames = {
    "true", "false", "cons", "nil", "pos", "negsuc"
};

struct MockMakeLcVar {
    MOCK_METHOD(const lc_expr*, make_var, (uint32_t index));
};

struct MockMakeLcAbs {
    MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr* body));
};

struct MockMakeLcApp {
    MOCK_METHOD(const lc_expr*, make_app, (const lc_expr* func, const lc_expr* arg));
};

struct TranspilerTest : public ::testing::Test {
    using transpiler_t             = transpiler<testing::NiceMock<MockMakeLcVar>,
                                               testing::NiceMock<MockMakeLcAbs>,
                                               testing::NiceMock<MockMakeLcApp>,
                                               scope, scope, scope>;
    using token_transpiler_t       = typename transpiler_t::token_transpiler_t;
    using abs_transpiler_t         = typename transpiler_t::abs_transpiler_t;
    using app_transpiler_t         = typename transpiler_t::app_transpiler_t;
    using binary_nat_transpiler_t  = typename transpiler_t::binary_nat_transpiler_t;
    using church_nat_transpiler_t  = typename transpiler_t::church_nat_transpiler_t;
    using integer_transpiler_t     = typename transpiler_t::integer_transpiler_t;
    using character_transpiler_t   = typename transpiler_t::character_transpiler_t;
    using string_transpiler_t      = typename transpiler_t::string_transpiler_t;
    using scott_list_transpiler_t  = typename transpiler_t::scott_list_transpiler_t;
    using church_list_transpiler_t = typename transpiler_t::church_list_transpiler_t;

    aml_expr_pool aml_pool;
    lc_expr_pool  lc_pool;
    scope         sc;
    testing::NiceMock<MockMakeLcVar> mock_var;
    testing::NiceMock<MockMakeLcAbs> mock_abs;
    testing::NiceMock<MockMakeLcApp> mock_app;

    // tx declared first: receives forward references to sub-components below.
    transpiler_t             tx;
    token_transpiler_t       token_;
    abs_transpiler_t         abs_;
    app_transpiler_t         app_;
    binary_nat_transpiler_t  binary_nat_;
    church_nat_transpiler_t  church_nat_;
    integer_transpiler_t     integer_;
    character_transpiler_t   character_;
    string_transpiler_t      string_;
    scott_list_transpiler_t  scott_list_;
    church_list_transpiler_t church_list_;

    TranspilerTest()
        : tx(token_, abs_, app_,
             binary_nat_, church_nat_,
             integer_, character_,
             string_,
             scott_list_, church_list_),
          token_(mock_var, sc),
          abs_(tx, mock_abs, sc, sc),
          app_(tx, mock_app),
          binary_nat_(mock_var, mock_app, sc),
          church_nat_(mock_var, mock_abs, mock_app),
          integer_(mock_var, mock_app, binary_nat_, sc),
          character_(binary_nat_),
          string_(binary_nat_, mock_var, mock_app, sc),
          scott_list_(tx, mock_var, mock_app, sc),
          church_list_(tx, mock_var, mock_abs, mock_app) {}

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc_pool.make_var(i);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc_pool.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc_pool.make_app(f, a);
        });
    }

    const lc_expr* lc_var(uint32_t i) { return lc_pool.make_var(i); }
    const lc_expr* lc_abs(const lc_expr* b) { return lc_pool.make_abs(b); }
    const lc_expr* lc_app(const lc_expr* f, const lc_expr* a) {
        return lc_pool.make_app(f, a);
    }

    const lc_expr* tx_expr(const aml_expr* e, const std::vector<std::string>& names) {
        for (const auto& n : names) sc.push(n);
        const lc_expr* result = tx.transpile(e);
        for (size_t i = 0; i < names.size(); ++i) sc.pop();
        return result;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Scope index alignment (replaces BuiltinIndicesAlignWithGlobalEnv)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, BuiltinIndicesAlignWithScope) {
    for (const auto& n : kBuiltinNames) sc.push(n);
    EXPECT_EQ(sc.get_var_index("true"),  5u);
    EXPECT_EQ(sc.get_var_index("false"), 4u);
    EXPECT_EQ(sc.get_var_index("cons"),  3u);
    EXPECT_EQ(sc.get_var_index("nil"),   2u);
    for (size_t i = 0; i < kBuiltinNames.size(); ++i) sc.pop();
}

// ---------------------------------------------------------------------------
// Core expression forms
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileIdentity) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_token("x"));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), lc_abs(lc_var(0)));
}

TEST_F(TranspilerTest, TranspileCurriedAbstraction) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y", aml_pool.make_token("x")));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), lc_abs(lc_abs(lc_var(1))));
}

TEST_F(TranspilerTest, TranspileApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_app(aml_pool.make_token("f"), aml_pool.make_token("x"))));
    const lc_expr* expected = lc_abs(lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileLeftFoldedApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_abs("y",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_token("f"), aml_pool.make_token("x")),
                    aml_pool.make_token("y")))));
    const lc_expr* expected = lc_abs(lc_abs(lc_abs(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileLocalShadowing) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("x"),
            aml_pool.make_abs("x", aml_pool.make_token("x"))));
    const lc_expr* expected = lc_abs(lc_app(lc_var(0), lc_abs(lc_var(0))));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, GlobalRefUnderNestedLambda) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_token("a"));
    const lc_expr* got = tx_expr(e, {"a", "b"});
    EXPECT_EQ(got, lc_abs(lc_var(2)));
}

TEST_F(TranspilerTest, UnboundNameThrows) {
    const aml_expr* e = aml_pool.make_token("missing");
    EXPECT_THROW(tx_expr(e, kBuiltinNames), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Literals (binary uses var(k); Church stays closed)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileNatZeroBinary) {
    const aml_expr* e = aml_pool.make_nat(0, nat_format::binary);
    EXPECT_EQ(tx_expr(e, kBuiltinNames), lc_var(2));
}

TEST_F(TranspilerTest, TranspileNatOneBinary) {
    const aml_expr* e = aml_pool.make_nat(1, nat_format::binary);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(5)), lc_var(2));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileNatTwoBinary) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::binary);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(4)),
        lc_app(lc_app(lc_var(3), lc_var(5)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileNatChurchTwo) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_var(1), lc_app(lc_var(1), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileIntZero) {
    const aml_expr* e = aml_pool.make_integer(0, nat_format::binary);
    const lc_expr* expected = lc_app(lc_var(1), lc_var(2));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileIntPositive) {
    const aml_expr* e = aml_pool.make_integer(12, nat_format::binary);
    const aml_expr* nat = aml_pool.make_nat(12, nat_format::binary);
    const lc_expr* expected = lc_app(lc_var(1), tx_expr(nat, kBuiltinNames));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileIntNegative) {
    const aml_expr* e = aml_pool.make_integer(-12, nat_format::binary);
    const aml_expr* nat = aml_pool.make_nat(11, nat_format::binary);
    const lc_expr* expected = lc_app(lc_var(0), tx_expr(nat, kBuiltinNames));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileChar) {
    const aml_expr* e = aml_pool.make_character('A');
    const aml_expr* as_nat = aml_pool.make_nat(65, nat_format::binary);
    EXPECT_EQ(tx_expr(e, kBuiltinNames), tx_expr(as_nat, kBuiltinNames));
}

TEST_F(TranspilerTest, TranspileStringEmpty) {
    const aml_expr* e = aml_pool.make_string("");
    EXPECT_EQ(tx_expr(e, kBuiltinNames), lc_var(2));
}

TEST_F(TranspilerTest, TranspileStringHello) {
    const aml_expr* e = aml_pool.make_string("hi");
    const lc_expr* h = tx_expr(aml_pool.make_character('h'), kBuiltinNames);
    const lc_expr* i = tx_expr(aml_pool.make_character('i'), kBuiltinNames);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), h),
        lc_app(lc_app(lc_var(3), i), lc_var(2)));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileListScott) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* b = aml_pool.make_character('b');
    const aml_expr* e = aml_pool.make_list({a, b}, list_format::scott);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), tx_expr(a, kBuiltinNames)),
        lc_app(lc_app(lc_var(3), tx_expr(b, kBuiltinNames)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, TranspileListChurch) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* e = aml_pool.make_list({a}, list_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_app(lc_var(1), tx_expr(a, kBuiltinNames)), lc_var(0))));
    EXPECT_EQ(tx_expr(e, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, MissingScopeEntryThrowsOnScottList) {
    const aml_expr* e = aml_pool.make_list({aml_pool.make_integer(1, nat_format::binary)}, list_format::scott);
    EXPECT_THROW(tx_expr(e, {"nil"}), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Fragment transpilation (functions, mutual refs, no delta inlining)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, NotFunctionUsesGlobalIndices) {
    const aml_expr* body = aml_pool.make_abs("b",
        aml_pool.make_app(
            aml_pool.make_app(aml_pool.make_token("b"), aml_pool.make_token("false")),
            aml_pool.make_token("true")));
    const lc_expr* got = tx_expr(body, kBuiltinNames);
    const lc_expr* expected = lc_abs(lc_app(
        lc_app(lc_var(0), lc_var(5)), lc_var(6)));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, IfThenElseFunction) {
    const aml_expr* body = aml_pool.make_abs("cond",
        aml_pool.make_abs("a",
            aml_pool.make_abs("b",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_token("cond"), aml_pool.make_token("a")),
                    aml_pool.make_token("b")))));
    const lc_expr* expected = lc_abs(lc_abs(lc_abs(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(body, kBuiltinNames), expected);
}

TEST_F(TranspilerTest, MutualDefsSameScope) {
    const std::vector<std::string> names = {"f", "g"};
    const aml_expr* f_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("g"), aml_pool.make_token("x")));
    const aml_expr* g_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("f"), aml_pool.make_token("x")));
    const lc_expr* f_got = tx_expr(f_body, names);
    const lc_expr* g_got = tx_expr(g_body, names);
    EXPECT_EQ(f_got, lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(g_got, lc_abs(lc_app(lc_var(2), lc_var(0))));
}

TEST_F(TranspilerTest, ComposeIdIdNoDoubleInlining) {
    const std::vector<std::string> names = [] {
        auto v = kBuiltinNames;
        v.push_back("compose");
        v.push_back("id");
        return v;
    }();
    const aml_expr* main = aml_pool.make_app(
        aml_pool.make_app(aml_pool.make_token("compose"), aml_pool.make_token("id")),
        aml_pool.make_token("id"));
    const lc_expr* got = tx_expr(main, names);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(1), lc_var(0)), lc_var(0));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, TranspileIdentityFunctionFragment) {
    const aml_expr* id = aml_pool.make_abs("x", aml_pool.make_token("x"));
    EXPECT_EQ(tx_expr(id, {"id"}), lc_abs(lc_var(0)));
}

// ---------------------------------------------------------------------------
// 3-level nested abstraction — de Bruijn indices at depth 3
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileTriplyCurriedOutermostVar) {
    // λx.λy.λz.x — x is under 3 binders, index = 2
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_token("x"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(2)))));
}

TEST_F(TranspilerTest, TranspileTriplyCurriedMiddleVar) {
    // λx.λy.λz.y — y is under 2 binders below it, index = 1
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_token("y"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(1)))));
}

TEST_F(TranspilerTest, TranspileTriplyCurriedInnermostVar) {
    // λx.λy.λz.z — z is the most recently bound, index = 0
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_token("z"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(0)))));
}
