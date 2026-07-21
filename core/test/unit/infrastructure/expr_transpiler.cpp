// Transpiler unit tests: aml_expr -> lc_expr with pure LC name resolution.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/builtin_transpiler.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"
#include "value_objects/bool_decl_group.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/int_decl_group.hpp"

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
    using decl_t = declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool>;
    using builtin_t = builtin_transpiler<decl_t>;
    using transpiler_t = transpiler<testing::NiceMock<MockMakeLcVar>,
                                    testing::NiceMock<MockMakeLcAbs>,
                                    testing::NiceMock<MockMakeLcApp>,
                                    scope, scope, scope, builtin_t>;
    using symbol_transpiler_t    = typename transpiler_t::symbol_transpiler_t;
    using abs_transpiler_t       = typename transpiler_t::abs_transpiler_t;
    using app_transpiler_t       = typename transpiler_t::app_transpiler_t;
    using nat_transpiler_t       = typename transpiler_t::nat_transpiler_t;
    using integer_transpiler_t   = typename transpiler_t::integer_transpiler_t;
    using character_transpiler_t = typename transpiler_t::character_transpiler_t;
    using string_transpiler_t    = typename transpiler_t::string_transpiler_t;
    using list_transpiler_t      = typename transpiler_t::list_transpiler_t;

    aml_expr_pool aml_pool;
    lc_expr_pool  lc_pool;
    scope         sc;
    testing::NiceMock<MockMakeLcVar> mock_var;
    testing::NiceMock<MockMakeLcAbs> mock_abs;
    testing::NiceMock<MockMakeLcApp> mock_app;

    decl_t                   decl;
    builtin_t                builtin_;
    transpiler_t             tx;
    symbol_transpiler_t      symbol_;
    abs_transpiler_t         abs_;
    app_transpiler_t         app_;
    nat_transpiler_t         nat_;
    integer_transpiler_t     integer_;
    character_transpiler_t   character_;
    string_transpiler_t      string_;
    list_transpiler_t        list_;

    TranspilerTest()
        : decl(lc_pool, lc_pool, lc_pool)
        , builtin_(decl)
        , tx(symbol_, abs_, app_,
             nat_,
             integer_, character_,
             string_,
             list_),
          symbol_(mock_var, sc, sc, builtin_),
          abs_(tx, mock_abs, sc, sc),
          app_(tx, mock_app),
          nat_(mock_var, mock_abs, mock_app, builtin_, builtin_, builtin_, builtin_),
          integer_(mock_app, nat_, builtin_, builtin_),
          character_(nat_),
          string_(nat_, mock_app, builtin_, builtin_),
          list_(tx, mock_var, mock_abs, mock_app, builtin_, builtin_) {}

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

TEST_F(TranspilerTest, TranspileIdentity) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_symbol("x"));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_var(0)));
}

TEST_F(TranspilerTest, TranspileCurriedAbstraction) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y", aml_pool.make_symbol("x")));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_var(1))));
}

TEST_F(TranspilerTest, TranspileApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_app(aml_pool.make_symbol("f"), aml_pool.make_symbol("x"))));
    const lc_expr* expected = lc_abs(lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileLeftFoldedApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_abs("y",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_symbol("f"), aml_pool.make_symbol("x")),
                    aml_pool.make_symbol("y")))));
    const lc_expr* expected = lc_abs(lc_abs(lc_abs(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileLocalShadowing) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_symbol("x"),
            aml_pool.make_abs("x", aml_pool.make_symbol("x"))));
    const lc_expr* expected = lc_abs(lc_app(lc_var(0), lc_abs(lc_var(0))));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, GlobalRefUnderNestedLambda) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_symbol("a"));
    const lc_expr* got = tx_expr(e, {"a", "b"});
    EXPECT_EQ(got, lc_abs(lc_var(2)));
}

TEST_F(TranspilerTest, UnboundNameThrows) {
    const aml_expr* e = aml_pool.make_symbol("missing");
    EXPECT_THROW(tx_expr(e, {}), std::out_of_range);
}

TEST_F(TranspilerTest, BuiltinTrueWithoutScopeIsClosedScott) {
    const aml_expr* e = aml_pool.make_symbol("true");
    EXPECT_EQ(tx_expr(e, {}), builtin_.transpile_true());
}

TEST_F(TranspilerTest, ScopeWinsOverBuiltinTrue) {
    const aml_expr* e = aml_pool.make_symbol("true");
    EXPECT_EQ(tx_expr(e, {"true"}), lc_var(0));
}

TEST_F(TranspilerTest, TranspileNatZeroBinary) {
    const aml_expr* e = aml_pool.make_nat(0, nat_format::binary);
    EXPECT_EQ(tx_expr(e, {}), builtin_.transpile_nil());
}

TEST_F(TranspilerTest, TranspileNatOneBinary) {
    const aml_expr* e = aml_pool.make_nat(1, nat_format::binary);
    const lc_expr* expected = lc_app(
        lc_app(builtin_.transpile_cons(), builtin_.transpile_true()),
        builtin_.transpile_nil());
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileNatTwoBinary) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::binary);
    const lc_expr* expected = lc_app(
        lc_app(builtin_.transpile_cons(), builtin_.transpile_false()),
        lc_app(lc_app(builtin_.transpile_cons(), builtin_.transpile_true()),
               builtin_.transpile_nil()));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileNatChurchTwo) {
    const aml_expr* e = aml_pool.make_nat(2, nat_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_var(1), lc_app(lc_var(1), lc_var(0)))));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileIntZero) {
    const aml_expr* e = aml_pool.make_integer(0, nat_format::binary);
    const lc_expr* expected = lc_app(builtin_.transpile_pos(), builtin_.transpile_nil());
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileIntPositive) {
    const aml_expr* e = aml_pool.make_integer(12, nat_format::binary);
    const aml_expr* nat = aml_pool.make_nat(12, nat_format::binary);
    const lc_expr* expected = lc_app(builtin_.transpile_pos(), tx_expr(nat, {}));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileIntNegative) {
    const aml_expr* e = aml_pool.make_integer(-12, nat_format::binary);
    const aml_expr* nat = aml_pool.make_nat(11, nat_format::binary);
    const lc_expr* expected = lc_app(builtin_.transpile_negsuc(), tx_expr(nat, {}));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileChar) {
    const aml_expr* e = aml_pool.make_character('A');
    const aml_expr* as_nat = aml_pool.make_nat(65, nat_format::binary);
    EXPECT_EQ(tx_expr(e, {}), tx_expr(as_nat, {}));
}

TEST_F(TranspilerTest, TranspileStringEmpty) {
    const aml_expr* e = aml_pool.make_string("");
    EXPECT_EQ(tx_expr(e, {}), builtin_.transpile_nil());
}

TEST_F(TranspilerTest, TranspileStringHello) {
    const aml_expr* e = aml_pool.make_string("hi");
    const lc_expr* h = tx_expr(aml_pool.make_character('h'), {});
    const lc_expr* i = tx_expr(aml_pool.make_character('i'), {});
    const lc_expr* expected = lc_app(
        lc_app(builtin_.transpile_cons(), h),
        lc_app(lc_app(builtin_.transpile_cons(), i), builtin_.transpile_nil()));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileListScott) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* b = aml_pool.make_character('b');
    const aml_expr* e = aml_pool.make_list({a, b}, list_format::scott);
    const lc_expr* expected = lc_app(
        lc_app(builtin_.transpile_cons(), tx_expr(a, {})),
        lc_app(lc_app(builtin_.transpile_cons(), tx_expr(b, {})), builtin_.transpile_nil()));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, TranspileListChurch) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* e = aml_pool.make_list({a}, list_format::church);
    const lc_expr* expected = lc_abs(lc_abs(
        lc_app(lc_app(lc_var(1), tx_expr(a, {})), lc_var(0))));
    EXPECT_EQ(tx_expr(e, {}), expected);
}

TEST_F(TranspilerTest, UnboundElementInListThrows) {
    const aml_expr* e = aml_pool.make_list(
        {aml_pool.make_symbol("missing")}, list_format::scott);
    EXPECT_THROW(tx_expr(e, {}), std::out_of_range);
}

TEST_F(TranspilerTest, NotFunctionUsesBuiltinFalseTrue) {
    const aml_expr* body = aml_pool.make_abs("b",
        aml_pool.make_app(
            aml_pool.make_app(aml_pool.make_symbol("b"), aml_pool.make_symbol("false")),
            aml_pool.make_symbol("true")));
    const lc_expr* got = tx_expr(body, {});
    const lc_expr* expected = lc_abs(lc_app(
        lc_app(lc_var(0), builtin_.transpile_false()), builtin_.transpile_true()));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, IfThenElseFunction) {
    const aml_expr* body = aml_pool.make_abs("cond",
        aml_pool.make_abs("a",
            aml_pool.make_abs("b",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_symbol("cond"), aml_pool.make_symbol("a")),
                    aml_pool.make_symbol("b")))));
    const lc_expr* expected = lc_abs(lc_abs(lc_abs(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_EQ(tx_expr(body, {}), expected);
}

TEST_F(TranspilerTest, MutualDefsSameScope) {
    const std::vector<std::string> names = {"f", "g"};
    const aml_expr* f_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_symbol("g"), aml_pool.make_symbol("x")));
    const aml_expr* g_body = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_symbol("f"), aml_pool.make_symbol("x")));
    const lc_expr* f_got = tx_expr(f_body, names);
    const lc_expr* g_got = tx_expr(g_body, names);
    EXPECT_EQ(f_got, lc_abs(lc_app(lc_var(1), lc_var(0))));
    EXPECT_EQ(g_got, lc_abs(lc_app(lc_var(2), lc_var(0))));
}

TEST_F(TranspilerTest, ComposeIdIdNoDoubleInlining) {
    const std::vector<std::string> names = {"compose", "id"};
    const aml_expr* main = aml_pool.make_app(
        aml_pool.make_app(aml_pool.make_symbol("compose"), aml_pool.make_symbol("id")),
        aml_pool.make_symbol("id"));
    const lc_expr* got = tx_expr(main, names);
    const lc_expr* expected = lc_app(
        lc_app(lc_var(1), lc_var(0)), lc_var(0));
    EXPECT_EQ(got, expected);
}

TEST_F(TranspilerTest, TranspileIdentityFunctionFragment) {
    const aml_expr* id = aml_pool.make_abs("x", aml_pool.make_symbol("x"));
    EXPECT_EQ(tx_expr(id, {"id"}), lc_abs(lc_var(0)));
}

TEST_F(TranspilerTest, TranspileTriplyCurriedOutermostVar) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_symbol("x"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(2)))));
}

TEST_F(TranspilerTest, TranspileTriplyCurriedMiddleVar) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_symbol("y"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(1)))));
}

TEST_F(TranspilerTest, TranspileTriplyCurriedInnermostVar) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y",
            aml_pool.make_abs("z", aml_pool.make_symbol("z"))));
    EXPECT_EQ(tx_expr(e, {}), lc_abs(lc_abs(lc_abs(lc_var(0)))));
}
