// Transpiler unit tests: aml_expr -> lc_expr with pure LC name resolution.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/global_env_factory.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/transpiler.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/list_format.hpp"

namespace {

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
    aml_expr_pool aml_pool;
    lc_expr_pool  lc_pool;
    MockMakeLcVar mock_var;
    MockMakeLcAbs mock_abs;
    MockMakeLcApp mock_app;
    transpiler<MockMakeLcVar, MockMakeLcAbs, MockMakeLcApp> tx{mock_var, mock_abs, mock_app};
    global_env builtins = global_env_from_builtin_names();
    local_binding_env empty_local;

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

    const lc_expr* tx_expr(const aml_expr* e, const global_env& global) {
        return tx.transpile(e, empty_local, global);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Builtin global_env alignment
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, BuiltinIndicesAlignWithGlobalEnv) {
    auto k_true = builtins.lookup_global("true");
    auto k_false = builtins.lookup_global("false");
    auto k_cons = builtins.lookup_global("cons");
    auto k_nil = builtins.lookup_global("nil");
    ASSERT_TRUE(k_true.has_value());
    ASSERT_TRUE(k_false.has_value());
    ASSERT_TRUE(k_cons.has_value());
    ASSERT_TRUE(k_nil.has_value());
    EXPECT_EQ(*k_true, 5u);
    EXPECT_EQ(*k_false, 4u);
    EXPECT_EQ(*k_cons, 3u);
    EXPECT_EQ(*k_nil, 2u);
}

// ---------------------------------------------------------------------------
// Core expression forms
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileIdentity) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_token("x"));
    EXPECT_EQ(tx_expr(e, builtins), lc_abs(lc_var(0)));
}

TEST_F(TranspilerTest, TranspileCurriedAbstraction) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y", aml_pool.make_token("x")));
    EXPECT_EQ(tx_expr(e, builtins), lc_abs(lc_abs(lc_var(1))));
}

TEST_F(TranspilerTest, TranspileApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_app(aml_pool.make_token("f"), aml_pool.make_token("x"))));
    const lc_expr* expected = lc_abs(lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
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
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileLocalShadowing) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("x"),
            aml_pool.make_abs("x", aml_pool.make_token("x"))));
    const lc_expr* expected = lc_abs(lc_app(lc_var(0), lc_abs(lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, GlobalRefUnderNestedLambda) {
    global_env env(std::vector<std::string>{"a", "b"});
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_token("a"));
    const lc_expr* got = tx_expr(e, env);
    EXPECT_EQ(got, lc_abs(lc_var(2)));
}

TEST_F(TranspilerTest, UnboundNameThrows) {
    const aml_expr* e = aml_pool.make_token("missing");
    EXPECT_THROW(tx_expr(e, builtins), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Literals (Scott uses var(k); Church stays closed)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileNatZeroScott) {
    const aml_expr* e = aml_pool.make_nat(0, nat_format::scott);
    EXPECT_EQ(tx_expr(e, builtins), lc_var(2));
}

TEST_F(TranspilerTest, TranspileNatOneScott) {
    const aml_expr* e = aml_pool.make_nat(1, nat_format::scott);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(5)), lc_var(2));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileNatTwoScott) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::scott);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(4)),
        lc_app(lc_app(lc_var(3), lc_var(5)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileNatChurchTwo) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_var(1), lc_app(lc_var(1), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileIntZero) {
    const aml_expr* e = aml_pool.make_integer(0);
    const lc_expr* expected = lc_app(lc_var(1), lc_var(2));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileIntPositive) {
    const aml_expr* e = aml_pool.make_integer(12);
    const aml_expr* nat = aml_pool.make_nat(12, nat_format::scott);
    const lc_expr* expected = lc_app(lc_var(1), tx_expr(nat, builtins));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileIntNegative) {
    const aml_expr* e = aml_pool.make_integer(-12);
    const aml_expr* nat = aml_pool.make_nat(11, nat_format::scott);
    const lc_expr* expected = lc_app(lc_var(0), tx_expr(nat, builtins));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileChar) {
    const aml_expr* e = aml_pool.make_character('A');
    const aml_expr* as_nat = aml_pool.make_nat(65, nat_format::scott);
    EXPECT_EQ(tx_expr(e, builtins), tx_expr(as_nat, builtins));
}

TEST_F(TranspilerTest, TranspileStringEmpty) {
    const aml_expr* e = aml_pool.make_string("");
    EXPECT_EQ(tx_expr(e, builtins), lc_var(2));
}

TEST_F(TranspilerTest, TranspileStringHello) {
    const aml_expr* e = aml_pool.make_string("hi");
    const lc_expr* h = tx_expr(aml_pool.make_character('h'), builtins);
    const lc_expr* i = tx_expr(aml_pool.make_character('i'), builtins);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), h),
        lc_app(lc_app(lc_var(3), i), lc_var(2)));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileListScott) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* b = aml_pool.make_character('b');
    const aml_expr* e = aml_pool.make_list({a, b}, list_format::scott);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), tx_expr(a, builtins)),
        lc_app(lc_app(lc_var(3), tx_expr(b, builtins)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileListChurch) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* e = aml_pool.make_list({a}, list_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_app(lc_var(1), tx_expr(a, builtins)), lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, MissingConsInGlobalEnvThrowsOnScottList) {
    global_env env(std::vector<std::string>{"nil"});
    const aml_expr* e = aml_pool.make_list({aml_pool.make_integer(1)}, list_format::scott);
    EXPECT_THROW(tx_expr(e, env), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Fragment transpilation (functions, mutual refs, no delta inlining)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, NotFunctionUsesGlobalIndices) {
    const aml_expr* body = aml_pool.make_abs("b",
        aml_pool.make_app(
            aml_pool.make_app(aml_pool.make_token("b"), aml_pool.make_token("false")),
            aml_pool.make_token("true")));
    const lc_expr* got = tx_expr(body, builtins);
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
    EXPECT_EQ(tx_expr(body, builtins), expected);
}

TEST_F(TranspilerTest, MutualDefsSameGlobalEnv) {
    global_env env(std::vector<std::string>{"f", "g"});
    const aml_expr* f_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("g"), aml_pool.make_token("x")));
    const aml_expr* g_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_token("f"), aml_pool.make_token("x")));
    const lc_expr* f_got = tx_expr(f_body, env);
    const lc_expr* g_got = tx_expr(g_body, env);
    EXPECT_EQ(f_got, lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(g_got, lc_abs(lc_app(lc_var(2), lc_var(0))));
}

TEST_F(TranspilerTest, ComposeIdIdNoDoubleInlining) {
    global_env env = global_env_from_builtin_names_and({"compose", "id"});
    const aml_expr* main = aml_pool.make_app(
        aml_pool.make_app(aml_pool.make_token("compose"), aml_pool.make_token("id")),
        aml_pool.make_token("id"));
    const lc_expr* got = tx_expr(main, env);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(1), lc_var(0)), lc_var(0));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, TranspileIdentityFunctionFragment) {
    global_env env(std::vector<std::string>{"id"});
    const aml_expr* id = aml_pool.make_abs("x", aml_pool.make_token("x"));
    EXPECT_EQ(tx_expr(id, env), lc_abs(lc_var(0)));
}
