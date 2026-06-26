// Stress tests for the aml -> lc transpiler.

#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/transpiler.hpp"

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

aml_program single_fn(const std::string& name, const aml_expr* body) {
    aml_program prog;
    prog.functions.push_back({name, body});
    return prog;
}

} // namespace

TEST_F(TranspilerStressTest, ManyNatLiterals) {
    lc_expr_pool lc;
    transpiler tx{lc};
    std::map<std::string, const lc_expr*> globals;
    tx.register_builtins(globals);

    for (uint64_t n = 0; n < 512; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, false);
        ASSERT_NE(tx.transpile(e, globals), nullptr);
    }
}

TEST_F(TranspilerStressTest, ManyChurchNats) {
    lc_expr_pool lc;
    transpiler tx{lc};
    std::map<std::string, const lc_expr*> globals;
    tx.register_builtins(globals);

    for (uint64_t n = 0; n < 64; ++n) {
        const aml_expr* e = aml_pool.make_nat(n, true);
        ASSERT_NE(tx.transpile(e, globals), nullptr);
    }
}

TEST_F(TranspilerStressTest, DeepAbstractionChain) {
    const aml_expr* e = build_deep_abs(aml_pool, 200);
    transpiled_program out = transpiler::transpile_program(single_fn("deep", e));
    EXPECT_NE(out.functions[0].body, nullptr);
}

TEST_F(TranspilerStressTest, DeepApplicationChain) {
    const aml_expr* e = build_deep_app(aml_pool, 100);
    transpiled_program out = transpiler::transpile_program(single_fn("deep_app", e));
    EXPECT_NE(out.functions[0].body, nullptr);
}

TEST_F(TranspilerStressTest, LargeScottList) {
    std::vector<const aml_expr*> elems;
    elems.reserve(256);
    for (int i = 0; i < 256; ++i)
        elems.push_back(aml_pool.make_character(static_cast<char>('a' + (i % 26))));
    const aml_expr* e = aml_pool.make_list(elems, false);
    transpiled_program out = transpiler::transpile_program(single_fn("big_list", e));
    EXPECT_NE(out.functions[0].body, nullptr);
}

TEST_F(TranspilerStressTest, ManyFunctionDefinitions) {
    aml_program prog;
    for (int i = 0; i < 100; ++i)
        prog.functions.push_back({"f" + std::to_string(i),
                                  aml_pool.make_abs("x", aml_pool.make_var("x"))});

    transpiled_program out = transpiler::transpile_program(prog);
    EXPECT_EQ(out.functions.size(), 100u);
    const lc_expr* id = out.pool.make_lam(out.pool.make_var(0));
    for (const auto& fn : out.functions)
        EXPECT_TRUE(lc_expr_eq(fn.body, id));
}

TEST_F(TranspilerStressTest, LargeProgramPoolGrowth) {
    aml_program prog;
    for (int i = 0; i < 50; ++i) {
        prog.functions.push_back(
            {"g" + std::to_string(i),
             aml_pool.make_app(aml_pool.make_var("true"), aml_pool.make_var("false"))});
    }

    transpiled_program out = transpiler::transpile_program(prog);
    EXPECT_GT(out.pool.size(), 10u);
    EXPECT_EQ(out.globals.count("true"), 1u);
}
