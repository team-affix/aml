// Stress tests for the aml -> lc transpiler (fragment-level).

#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/builtin_constructors.hpp"
#include "infrastructure/expr_transpiler.hpp"

namespace {

struct TranspilerStressTest : public ::testing::Test {
    aml_expr_pool aml_pool;
};

const aml_expr* build_deep_abs(aml_expr_pool& pool, uint32_t depth) {
    const aml_expr* body = pool.make_var("x");
    for (uint32_t i = 1; i < depth; ++i)
        body = pool.make_abs("x", body);
    return body;
}

const aml_expr* build_deep_app(aml_expr_pool& pool, uint32_t depth) {
    const aml_expr* body = pool.make_var("x");
    for (uint32_t i = 0; i < depth; ++i)
        body = pool.make_app(pool.make_var("f"), body);
    return pool.make_abs("f", pool.make_abs("x", body));
}

global_env stress_global_env() {
    return global_env_from_builtin_names();
}

} // namespace

TEST_F(TranspilerStressTest, ManyNatLiterals) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env = stress_global_env();
    local_binding_env local;

    for (uint64_t n = 0; n < 512; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, false);
        ASSERT_NE(tx.transpile_expr(e, local, env), nullptr);
    }
}

TEST_F(TranspilerStressTest, ManyChurchNats) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env = stress_global_env();
    local_binding_env local;

    for (uint64_t n = 0; n < 64; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, true);
        ASSERT_NE(tx.transpile_expr(e, local, env), nullptr);
    }
}

TEST_F(TranspilerStressTest, DeepAbstractionChain) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env(std::vector<std::string>{"deep"});
    local_binding_env local;
    const aml_expr* e = build_deep_abs(aml_pool, 200);
    EXPECT_NE(tx.transpile_expr(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, DeepApplicationChain) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env = stress_global_env();
    local_binding_env local;
    const aml_expr* e = build_deep_app(aml_pool, 100);
    EXPECT_NE(tx.transpile_expr(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, LargeScottList) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env = stress_global_env();
    local_binding_env local;
    std::vector<const aml_expr*> elems;
    elems.reserve(256);
    for (int i = 0; i < 256; ++i)
        elems.push_back(aml_pool.make_character(static_cast<char>('a' + (i % 26))));
    const aml_expr* e = aml_pool.make_list(elems, false);
    EXPECT_NE(tx.transpile_expr(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, ManyFunctionFragments) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    std::vector<std::string> names;
    for (int i = 0; i < 100; ++i)
        names.push_back("f" + std::to_string(i));
    global_env env(std::move(names));
    local_binding_env local;
    const lc_expr* id = lc.make_lam(lc.make_var(0));
    for (int i = 0; i < 100; ++i) {
        const aml_expr* body = aml_pool.make_abs("x", aml_pool.make_var("x"));
        EXPECT_EQ(tx.transpile_expr(body, local, env), id);
    }
}

TEST_F(TranspilerStressTest, ManyGlobalRefFragments) {
    lc_expr_pool lc;
    expr_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> tx{lc, lc, lc};
    global_env env = stress_global_env();
    local_binding_env local;
    for (int i = 0; i < 50; ++i) {
        const aml_expr* e = aml_pool.make_app(
            aml_pool.make_var("true"), aml_pool.make_var("false"));
        ASSERT_NE(tx.transpile_expr(e, local, env), nullptr);
    }
    EXPECT_GT(lc.size(), 2u);
}
