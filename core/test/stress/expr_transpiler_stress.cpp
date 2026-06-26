// Stress tests for the aml -> lc transpiler (fragment-level).

#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/global_env_factory.hpp"
#include "infrastructure/lc_transpile_bundle.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct TranspilerStressTest : public ::testing::Test {
    aml_expr_pool aml_pool;
};

const aml_expr* build_deep_abs(aml_expr_pool& pool, uint32_t depth) {
    const aml_expr* body = pool.make_token("x");
    for (uint32_t i = 1; i < depth; ++i)
        body = pool.make_abs("x", body);
    return body;
}

const aml_expr* build_deep_app(aml_expr_pool& pool, uint32_t depth) {
    const aml_expr* body = pool.make_token("x");
    for (uint32_t i = 0; i < depth; ++i)
        body = pool.make_app(pool.make_token("f"), body);
    return pool.make_abs("f", pool.make_abs("x", body));
}

global_env stress_global_env() {
    return global_env_from_builtin_names();
}

} // namespace

TEST_F(TranspilerStressTest, ManyNatLiterals) {
    lc_transpile_bundle bundle;
    global_env env = stress_global_env();
    local_binding_env local;

    for (uint64_t n = 0; n < 512; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, nat_format::scott);
        ASSERT_NE(bundle.tx.transpile(e, local, env), nullptr);
    }
}

TEST_F(TranspilerStressTest, ManyChurchNats) {
    lc_transpile_bundle bundle;
    global_env env = stress_global_env();
    local_binding_env local;

    for (uint64_t n = 0; n < 64; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, nat_format::church);
        ASSERT_NE(bundle.tx.transpile(e, local, env), nullptr);
    }
}

TEST_F(TranspilerStressTest, DeepAbstractionChain) {
    lc_transpile_bundle bundle;
    global_env env(std::vector<std::string>{"deep"});
    local_binding_env local;
    const aml_expr* e = build_deep_abs(aml_pool, 200);
    EXPECT_NE(bundle.tx.transpile(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, DeepApplicationChain) {
    lc_transpile_bundle bundle;
    global_env env = stress_global_env();
    local_binding_env local;
    const aml_expr* e = build_deep_app(aml_pool, 100);
    EXPECT_NE(bundle.tx.transpile(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, LargeScottList) {
    lc_transpile_bundle bundle;
    global_env env = stress_global_env();
    local_binding_env local;
    std::vector<const aml_expr*> elems;
    elems.reserve(256);
    for (int i = 0; i < 256; ++i)
        elems.push_back(aml_pool.make_character(static_cast<char>('a' + (i % 26))));
    const aml_expr* e = aml_pool.make_list(elems, list_format::scott);
    EXPECT_NE(bundle.tx.transpile(e, local, env), nullptr);
}

TEST_F(TranspilerStressTest, ManyFunctionFragments) {
    lc_transpile_bundle bundle;
    std::vector<std::string> names;
    for (int i = 0; i < 100; ++i)
        names.push_back("f" + std::to_string(i));
    global_env env(std::move(names));
    local_binding_env local;
    const lc_expr* id = bundle.lc.make_lam(bundle.lc.make_var(0));
    for (int i = 0; i < 100; ++i) {
        const aml_expr* body = aml_pool.make_abs("x", aml_pool.make_token("x"));
        EXPECT_EQ(bundle.tx.transpile(body, local, env), id);
    }
}

TEST_F(TranspilerStressTest, ManyGlobalRefFragments) {
    lc_transpile_bundle bundle;
    global_env env = stress_global_env();
    local_binding_env local;
    for (int i = 0; i < 50; ++i) {
        const aml_expr* e = aml_pool.make_app(
            aml_pool.make_token("true"), aml_pool.make_token("false"));
        ASSERT_NE(bundle.tx.transpile(e, local, env), nullptr);
    }
    EXPECT_GT(bundle.lc.size(), 2u);
}
