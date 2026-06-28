// Stress tests for the aml -> lc transpiler (fragment-level).

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/transpiler_manifest.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/list_format.hpp"

namespace {

static const std::vector<std::string> kBuiltinNames = {
    "true", "false", "cons", "nil", "pos", "negsuc"
};

struct TranspilerStressTest : public ::testing::Test {
    aml_expr_pool aml_pool;

    const lc_expr* transpile_with(transpiler_manifest& bundle, const aml_expr* e,
                                  const std::vector<std::string>& names) {
        for (const auto& n : names) bundle.sc.push(n);
        const lc_expr* result = bundle.tx.transpile(e);
        for (size_t i = 0; i < names.size(); ++i) bundle.sc.pop();
        return result;
    }

    const lc_expr* transpile_with_builtins(transpiler_manifest& bundle, const aml_expr* e) {
        return transpile_with(bundle, e, kBuiltinNames);
    }
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

} // namespace

TEST_F(TranspilerStressTest, ManyNatLiterals) {
    transpiler_manifest bundle;
    for (uint64_t n = 0; n < 512; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, nat_format::binary);
        ASSERT_NE(transpile_with_builtins(bundle, e), nullptr);
    }
}

TEST_F(TranspilerStressTest, ManyChurchNats) {
    transpiler_manifest bundle;
    for (uint64_t n = 0; n < 64; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, nat_format::church);
        ASSERT_NE(transpile_with_builtins(bundle, e), nullptr);
    }
}

TEST_F(TranspilerStressTest, DeepAbstractionChain) {
    transpiler_manifest bundle;
    const aml_expr* e = build_deep_abs(aml_pool, 200);
    EXPECT_NE(transpile_with(bundle, e, {"deep"}), nullptr);
}

TEST_F(TranspilerStressTest, DeepApplicationChain) {
    transpiler_manifest bundle;
    const aml_expr* e = build_deep_app(aml_pool, 100);
    EXPECT_NE(transpile_with_builtins(bundle, e), nullptr);
}

TEST_F(TranspilerStressTest, LargeScottList) {
    transpiler_manifest bundle;
    std::vector<const aml_expr*> elems;
    elems.reserve(256);
    for (int i = 0; i < 256; ++i)
        elems.push_back(aml_pool.make_character(static_cast<char>('a' + (i % 26))));
    const aml_expr* e = aml_pool.make_list(elems, list_format::scott);
    EXPECT_NE(transpile_with_builtins(bundle, e), nullptr);
}

TEST_F(TranspilerStressTest, ManyFunctionFragments) {
    transpiler_manifest bundle;
    std::vector<std::string> names;
    for (int i = 0; i < 100; ++i)
        names.push_back("f" + std::to_string(i));
    const lc_expr* id = bundle.lc.make_abs(bundle.lc.make_var(0));
    for (int i = 0; i < 100; ++i) {
        const aml_expr* body = aml_pool.make_abs("x", aml_pool.make_token("x"));
        EXPECT_EQ(transpile_with(bundle, body, names), id);
    }
}

TEST_F(TranspilerStressTest, ManyGlobalRefFragments) {
    transpiler_manifest bundle;
    for (int i = 0; i < 50; ++i) {
        const aml_expr* e = aml_pool.make_app(
            aml_pool.make_token("true"), aml_pool.make_token("false"));
        ASSERT_NE(transpile_with_builtins(bundle, e), nullptr);
    }
    EXPECT_GT(bundle.lc.size(), 2u);
}
