// Transpiler unit tests: aml_expr -> lc_expr with Scott/Church encodings.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/transpiler.hpp"

namespace {

struct TranspilerTest : public ::testing::Test {
    aml_expr_pool aml_pool;
    lc_expr_pool  lc_pool;
    transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc_pool, lc_pool, lc_pool};
    std::map<std::string, const lc_expr*> globals;

    void SetUp() override { tx.register_builtins(globals); }

    const lc_expr* lc_var(uint32_t i) { return lc_pool.make_var(i); }
    const lc_expr* lc_lam(const lc_expr* b) { return lc_pool.make_lam(b); }
    const lc_expr* lc_app(const lc_expr* f, const lc_expr* a) {
        return lc_pool.make_app(f, a);
    }

    const lc_expr* tx_expr(const aml_expr* e) { return tx.transpile(e, globals); }
};

} // namespace

// ---------------------------------------------------------------------------
// Scott builtins
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, ScottTrue) {
    const lc_expr* t = globals.at("true");
    EXPECT_TRUE(lc_expr_eq(t, lc_lam(lc_lam(lc_var(1)))));
}

TEST_F(TranspilerTest, ScottFalse) {
    const lc_expr* f = globals.at("false");
    EXPECT_TRUE(lc_expr_eq(f, lc_lam(lc_lam(lc_var(0)))));
}

TEST_F(TranspilerTest, ScottNil) {
    const lc_expr* nil = globals.at("nil");
    EXPECT_TRUE(lc_expr_eq(nil, lc_lam(lc_lam(lc_var(0)))));
}

TEST_F(TranspilerTest, ScottConsHead) {
    const lc_expr* cons = globals.at("cons");
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(1), lc_var(3)), lc_var(2))))));
    EXPECT_TRUE(lc_expr_eq(cons, expected));
}

// ---------------------------------------------------------------------------
// Core expression forms
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileIdentity) {
    const aml_expr* e = aml_pool.make_abs("x", aml_pool.make_var("x"));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), lc_lam(lc_var(0))));
}

TEST_F(TranspilerTest, TranspileCurriedAbstraction) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_abs("y", aml_pool.make_var("x")));
    const lc_expr* got = tx_expr(e);
    EXPECT_TRUE(lc_expr_eq(got, lc_lam(lc_lam(lc_var(1)))));
}

TEST_F(TranspilerTest, TranspileApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_app(aml_pool.make_var("f"), aml_pool.make_var("x"))));
    const lc_expr* got = tx_expr(e);
    const lc_expr* expected = lc_lam(lc_lam(lc_app(lc_var(1), lc_var(0))));
    EXPECT_TRUE(lc_expr_eq(got, expected));
}

TEST_F(TranspilerTest, TranspileLeftFoldedApplication) {
    const aml_expr* e = aml_pool.make_abs("f",
        aml_pool.make_abs("x",
            aml_pool.make_abs("y",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_var("f"), aml_pool.make_var("x")),
                    aml_pool.make_var("y")))));
    const lc_expr* got = tx_expr(e);
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_TRUE(lc_expr_eq(got, expected));
}

TEST_F(TranspilerTest, TranspileLocalShadowing) {
    const aml_expr* e = aml_pool.make_abs("x",
        aml_pool.make_app(aml_pool.make_var("x"),
            aml_pool.make_abs("x", aml_pool.make_var("x"))));
    const lc_expr* got = tx_expr(e);
    const lc_expr* expected = lc_lam(lc_app(lc_var(0), lc_lam(lc_var(0))));
    EXPECT_TRUE(lc_expr_eq(got, expected));
}

TEST_F(TranspilerTest, UnboundNameThrows) {
    const aml_expr* e = aml_pool.make_var("missing");
    EXPECT_THROW(tx_expr(e), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Literals
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileNatZeroScott) {
    const aml_expr* e = aml_pool.make_nat(0, false);
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), globals.at("nil")));
}

TEST_F(TranspilerTest, TranspileNatOneScott) {
    const aml_expr* e = aml_pool.make_nat(1, false);
    const lc_expr* expected = lc_app(
        lc_app(globals.at("cons"), globals.at("true")), globals.at("nil"));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileNatTwoScott) {
    const aml_expr* e = aml_pool.make_nat(2, false);
    const lc_expr* expected = lc_app(
        lc_app(globals.at("cons"), globals.at("false")),
        lc_app(lc_app(globals.at("cons"), globals.at("true")), globals.at("nil")));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileNatChurchTwo) {
    const aml_expr* e = aml_pool.make_nat(2, true);
    const lc_expr* expected = lc_lam(lc_lam(
        lc_app(lc_var(1), lc_app(lc_var(1), lc_var(0)))));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileIntZero) {
    const aml_expr* e = aml_pool.make_integer(0);
    const lc_expr* expected = lc_app(globals.at("pos"), globals.at("nil"));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileIntPositive) {
    const aml_expr* e = aml_pool.make_integer(12);
    const aml_expr* nat = aml_pool.make_nat(12, false);
    const lc_expr* expected = lc_app(globals.at("pos"), tx_expr(nat));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileIntNegative) {
    const aml_expr* e = aml_pool.make_integer(-12);
    const aml_expr* nat = aml_pool.make_nat(11, false);
    const lc_expr* expected = lc_app(globals.at("negsuc"), tx_expr(nat));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileChar) {
    const aml_expr* e = aml_pool.make_character('A');
    const aml_expr* as_int = aml_pool.make_integer(65);
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), tx_expr(as_int)));
}

TEST_F(TranspilerTest, TranspileStringEmpty) {
    const aml_expr* e = aml_pool.make_string("");
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), globals.at("nil")));
}

TEST_F(TranspilerTest, TranspileStringHello) {
    const aml_expr* e = aml_pool.make_string("hi");
    const lc_expr* h = tx_expr(aml_pool.make_character('h'));
    const lc_expr* i = tx_expr(aml_pool.make_character('i'));
    const lc_expr* expected = lc_app(
        lc_app(globals.at("cons"), h),
        lc_app(lc_app(globals.at("cons"), i), globals.at("nil")));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileListScott) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* b = aml_pool.make_character('b');
    const aml_expr* e = aml_pool.make_list({a, b}, false);
    const lc_expr* expected = lc_app(
        lc_app(globals.at("cons"), tx_expr(a)),
        lc_app(lc_app(globals.at("cons"), tx_expr(b)), globals.at("nil")));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

TEST_F(TranspilerTest, TranspileListChurch) {
    const aml_expr* a = aml_pool.make_character('a');
    const aml_expr* e = aml_pool.make_list({a}, true);
    const lc_expr* expected = lc_lam(lc_lam(
        lc_app(lc_app(lc_var(1), tx_expr(a)), lc_var(0))));
    EXPECT_TRUE(lc_expr_eq(tx_expr(e), expected));
}

// ---------------------------------------------------------------------------
// Custom constructor groups
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileCustomConstructorGroup) {
    constructor_group group{{{"decided", 1}, {"undecided", 0}}};
    tx.register_group(group, globals);
    const lc_expr* undec = globals.at("undecided");
    EXPECT_TRUE(lc_expr_eq(undec, lc_lam(lc_lam(lc_var(0)))));
    const lc_expr* dec = globals.at("decided");
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_var(1), lc_var(2)))));
    EXPECT_TRUE(lc_expr_eq(dec, expected));
}

// ---------------------------------------------------------------------------
// Full program transpilation
// ---------------------------------------------------------------------------

TEST_F(TranspilerTest, TranspileProgramWithFunctions) {
    const aml_expr* id = aml_pool.make_abs("x", aml_pool.make_var("x"));
    const aml_expr* not_ = aml_pool.make_abs("b",
        aml_pool.make_app(
            aml_pool.make_app(aml_pool.make_var("b"), aml_pool.make_var("false")),
            aml_pool.make_var("true")));

    declaration_file decls;
    definition_file defs;
    defs.functions.push_back({"id", id});
    defs.functions.push_back({"not", not_});

    transpiled_program out = transpile_program(decls, defs);
    EXPECT_EQ(out.functions.size(), 2u);
    EXPECT_TRUE(lc_expr_eq(out.functions[0].body, lc_lam(lc_var(0))));
    EXPECT_NE(out.globals.at("true"), nullptr);
    EXPECT_NE(out.globals.at("false"), nullptr);
    EXPECT_TRUE(lc_expr_eq(out.globals.at("id"), out.functions[0].body));
    EXPECT_TRUE(lc_expr_eq(out.globals.at("not"), out.functions[1].body));
}

TEST_F(TranspilerTest, TranspileIfThenElseFunction) {
    const aml_expr* body = aml_pool.make_abs("cond",
        aml_pool.make_abs("a",
            aml_pool.make_abs("b",
                aml_pool.make_app(
                    aml_pool.make_app(aml_pool.make_var("cond"), aml_pool.make_var("a")),
                    aml_pool.make_var("b")))));

    definition_file defs;
    defs.functions.push_back({"if_then_else", body});
    transpiled_program out = transpile_program({}, defs);

    const lc_expr* got = out.functions[0].body;
    const lc_expr* expected = lc_lam(lc_lam(lc_lam(
        lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0)))));
    EXPECT_TRUE(lc_expr_eq(got, expected));
}
