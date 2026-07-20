// Integration tests: assembler + global_stack + lc_expr_pool.

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/assembler.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/global_stack.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/global.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement_file.hpp"

namespace {

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

struct AssemblerIntegrationTest : public ::testing::Test {
    lc_expr_pool lc;
    global_stack globals;
    assembler<lc_expr_pool, lc_expr_pool, global_stack> asm_{lc, lc, globals};
};

} // namespace

TEST_F(AssemblerIntegrationTest, EmptyGlobalsReturnsBody) {
    const lc_expr* body = lc.make_var(0);
    EXPECT_EQ(asm_.assemble(body), body);
}

TEST_F(AssemblerIntegrationTest, TwoGlobalsFirstDefinedIsOutermost) {
    const lc_expr* body = nullptr;
    const lc_expr* g0 = lc.make_var(0);
    const lc_expr* g1 = lc.make_var(1);
    globals.push(g0);
    globals.push(g1);

    const lc_expr* expected =
        lc.make_app(lc.make_abs(lc.make_app(lc.make_abs(body), g1)), g0);
    EXPECT_EQ(asm_.assemble(body), expected);
    EXPECT_TRUE(globals.empty());
}

TEST_F(AssemblerIntegrationTest, DefinitionOrderMatchesScopeOrder) {
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
    const lc_expr* true_term  = dt.transpile_decl(2u, 0u, 0u);
    const lc_expr* false_term = dt.transpile_decl(2u, 1u, 0u);

    globals.push(true_term);
    globals.push(false_term);

    const lc_expr* expected =
        lc.make_app(lc.make_abs(lc.make_app(lc.make_abs(nullptr), false_term)), true_term);
    EXPECT_EQ(asm_.assemble(nullptr), expected);
}

TEST_F(AssemblerIntegrationTest, BooleanDeclarationsAndNotDefinition) {
    aml_expr_pool aml;

    module_file mod;
    mod.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    const aml_expr* not_body = aml.make_abs("a",
        aml.make_app(
            aml.make_app(aml.make_token("a"), aml.make_token("false")),
            aml.make_token("true")));
    mod.items.push_back(global{definition{"not", not_body}});

    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts};
    while (auto g = em.global_it.get_next_global())
        em.global_proc.process_global(*g);

    assembler<lc_expr_pool, lc_expr_pool, global_stack>
        assemble{em.lc, em.lc, em.globals};
    const lc_expr* result = assemble.assemble(nullptr);
    ASSERT_NE(result, nullptr);

    ASSERT_TRUE(std::holds_alternative<lc_expr::app>(result->content));
    const auto& outer = std::get<lc_expr::app>(result->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.func->content));

    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(outer.arg->content));
    const auto& true_outer_abs = std::get<lc_expr::abs>(outer.arg->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::abs>(true_outer_abs.body->content));
    const auto& true_inner_abs = std::get<lc_expr::abs>(true_outer_abs.body->content);
    ASSERT_TRUE(std::holds_alternative<lc_expr::var>(true_inner_abs.body->content));
    EXPECT_EQ(std::get<lc_expr::var>(true_inner_abs.body->content).index, 1u);
}
