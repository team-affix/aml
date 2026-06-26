// Transpiler unit tests: aml_expr -> lc_expr with pure LC name resolution.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/builtin_constructors.hpp"
#include "infrastructure/expr_transpiler.hpp"
#include "infrastructure/scott_encoder.hpp"

namespace {

const lc_expr* find_binding(const std::vector<encoded_binding>& bindings,
                            const std::string& name) {
    for (const encoded_binding& b : bindings) {
        if (b.name == name)
            return b.definition;
    }
    throw std::runtime_error("binding not found: " + name);
}

struct TranspilerTest : public ::testing::Test {
    aml_expr_pool aml_pool;
    lc_expr_pool  lc_pool;
    scott_encoder<lc_expr_pool, lc_expr_pool, lc_expr_pool> encoder{lc_pool, lc_pool, lc_pool};
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc_pool, lc_pool, lc_pool};
    global_env builtins = global_env_from_builtin_names();
    local_binding_env empty_local;

    const lc_expr* lc_var(uint32_t i) { return lc_pool.make_var(i); }
    const lc_expr* lc_lam(const lc_expr* b) { return lc_pool.make_lam(b); }
    const lc_expr* lc_app(const lc_expr* f, const lc_expr* a) {
        return lc_pool.make_app(f, a);
    }

    const lc_expr* tx_expr(const aml_expr* e, const global_env& global) {
        return tx.transpile_expr(e, empty_local, global);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Scott builtins (encode_builtins + global_env alignment)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, ScottTrue) {
    const lc_expr* t = find_binding(encoder.encode_builtins(), "true");
    EXPECT_EQ(t, lc_lam(lc_lam(lc_var(1))));
}

TEST_F(TranspilerTest, ScottFalse) {
    const lc_expr* f = find_binding(encoder.encode_builtins(), "false");
    EXPECT_EQ(f, lc_lam(lc_lam(lc_var(0))));
}

TEST_F(TranspilerTest, ScottNil) {
    const lc_expr* nil = find_binding(encoder.encode_builtins(), "nil");
    EXPECT_EQ(nil, lc_lam(lc_lam(lc_var(0))));
}

TEST_F(TranspilerTest, ScottConsHead) {
    const lc_expr* cons = find_binding(encoder.encode_builtins(), "cons");
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(1), lc_var(3)), lc_var(2))))));
    EXPECT_EQ(cons, expected);
}

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
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_var("x"));
    EXPECT_EQ(tx_expr(e, builtins), lc_lam(lc_var(0)));
}

TEST_F(TranspilerTest, TranspileCurriedAbstraction) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y", aml_pool.make_var("x")));
    EXPECT_EQ(tx_expr(e, builtins), lc_lam(lc_lam(lc_var(1))));
}

TEST_F(TranspilerTest, TranspileApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_app(aml_pool.make_var("f"), aml_pool.make_var("x"))));
    const lc_expr* expected = lc_lam(lc_lam(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileLeftFoldedApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_abs("y",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_var("f"), aml_pool.make_var("x")),
                    aml_pool.make_var("y")))));
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileLocalShadowing) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_var("x"),
            aml_pool.make_abs("x", aml_pool.make_var("x"))));
    const lc_expr* expected = lc_lam(lc_app(lc_var(0), lc_lam(lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, GlobalRefUnderNestedLambda) {
    global_env env(std::vector<std::string>{"a", "b"});
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_var("a"));
    const lc_expr* got = tx_expr(e, env);
    EXPECT_EQ(got, lc_lam(lc_var(2)));
}

TEST_F(TranspilerTest, UnboundNameThrows) {
    const aml_expr* e = aml_pool.make_var("missing");
    EXPECT_THROW(tx_expr(e, builtins), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Literals (Scott uses var(k); Church stays closed)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileNatZeroScott) {
    const aml_expr* e = aml_pool.make_nat(0, false);
    EXPECT_EQ(tx_expr(e, builtins), lc_var(2));
}

TEST_F(TranspilerTest, TranspileNatOneScott) {
    const aml_expr* e = aml_pool.make_nat(1, false);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(5)), lc_var(2));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileNatTwoScott) {
    const aml_expr* e = aml_pool.make_nat(2, false);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), lc_var(4)),
        lc_app(lc_app(lc_var(3), lc_var(5)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileNatChurchTwo) {
    const aml_expr* e = aml_pool.make_nat(2, true);
    const lc_expr* expected = lc_lam(lc_lam(
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
    const aml_expr* nat = aml_pool.make_nat(12, false);
    const lc_expr* expected = lc_app(lc_var(1), tx_expr(nat, builtins));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileIntNegative) {
    const aml_expr* e = aml_pool.make_integer(-12);
    const aml_expr* nat = aml_pool.make_nat(11, false);
    const lc_expr* expected = lc_app(lc_var(0), tx_expr(nat, builtins));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileChar) {
    const aml_expr* e = aml_pool.make_character('A');
    const aml_expr* as_int = aml_pool.make_integer(65);
    EXPECT_EQ(tx_expr(e, builtins), tx_expr(as_int, builtins));
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
    const aml_expr* e = aml_pool.make_list({a, b}, false);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(3), tx_expr(a, builtins)),
        lc_app(lc_app(lc_var(3), tx_expr(b, builtins)), lc_var(2)));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, TranspileListChurch) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* e = aml_pool.make_list({a}, true);
    const lc_expr* expected = lc_lam(lc_lam(
        lc_app(lc_app(lc_var(1), tx_expr(a, builtins)), lc_var(0))));
    EXPECT_EQ(tx_expr(e, builtins), expected);
}

TEST_F(TranspilerTest, MissingConsInGlobalEnvThrowsOnScottList) {
    global_env env(std::vector<std::string>{"nil"});
    const aml_expr* e = aml_pool.make_list({aml_pool.make_integer(1)}, false);
    EXPECT_THROW(tx_expr(e, env), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Custom constructor groups
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileCustomConstructorGroup) {
    constructor_group group{{{"decided", 1}, {"undecided", 0}}};
    auto encoded = encoder.encode_constructor_group(group);
    const lc_expr* undec = find_binding(encoded, "undecided");
    EXPECT_EQ(undec, lc_lam(lc_lam(lc_var(0))));
    const lc_expr* dec = find_binding(encoded, "decided");
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_var(1), lc_var(2)))));
    EXPECT_EQ(dec, expected);
}

// ---------------------------------------------------------------------------
// Fragment transpilation (functions, mutual refs, no delta inlining)
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, NotFunctionUsesGlobalIndices) {
    const aml_expr* body = aml_pool.make_abs("b",
        aml_pool.make_app(
            aml_pool.make_app(aml_pool.make_var("b"), aml_pool.make_var("false")),
            aml_pool.make_var("true")));
    const lc_expr* got = tx_expr(body, builtins);
    const lc_expr* expected = lc_lam(lc_app(
        lc_app(lc_var(0), lc_var(5)), lc_var(6)));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, IfThenElseFunction) {
    const aml_expr* body = aml_pool.make_abs("cond",
        aml_pool.make_abs("a",
            aml_pool.make_abs("b",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_var("cond"), aml_pool.make_var("a")),
                    aml_pool.make_var("b")))));
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(body, builtins), expected);
}

TEST_F(TranspilerTest, MutualDefsSameGlobalEnv) {
    global_env env(std::vector<std::string>{"f", "g"});
    const aml_expr* f_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_var("g"), aml_pool.make_var("x")));
    const aml_expr* g_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_var("f"), aml_pool.make_var("x")));
    const lc_expr* f_got = tx_expr(f_body, env);
    const lc_expr* g_got = tx_expr(g_body, env);
    EXPECT_EQ(f_got, lc_lam(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(g_got, lc_lam(lc_app(lc_var(2), lc_var(0))));
}

TEST_F(TranspilerTest, ComposeIdIdNoDoubleInlining) {
    global_env env({
        "true", "false", "cons", "nil", "pos", "negsuc", "compose", "id",
    });
    const aml_expr* main = aml_pool.make_app(
        aml_pool.make_app(aml_pool.make_var("compose"), aml_pool.make_var("id")),
        aml_pool.make_var("id"));
    const lc_expr* got = tx_expr(main, env);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(1), lc_var(0)), lc_var(0));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, TranspileIdentityFunctionFragment) {
    global_env env(std::vector<std::string>{"id"});
    const aml_expr* id = aml_pool.make_abs("x", aml_pool.make_var("x"));
    EXPECT_EQ(tx_expr(id, env), lc_lam(lc_var(0)));
}
