// Integration tests for elaborator_manifest global/statement processing.

#include <stdexcept>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/global.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

namespace {

static bool struct_eq(const lc_expr* a, const lc_expr* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->content.index() != b->content.index()) return false;
    if (auto* va = std::get_if<lc_expr::var>(&a->content))
        return *va == std::get<lc_expr::var>(b->content);
    if (auto* aa = std::get_if<lc_expr::abs>(&a->content))
        return struct_eq(aa->body, std::get<lc_expr::abs>(b->content).body);
    const auto& ap_a = std::get<lc_expr::app>(a->content);
    const auto& ap_b = std::get<lc_expr::app>(b->content);
    return struct_eq(ap_a.func, ap_b.func) && struct_eq(ap_a.arg, ap_b.arg);
}

static declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = std::vector<declaration>(decls);
    return g;
}

static const lc_expr* assemble_lc(lc_expr_pool& lc,
                                  const std::vector<const lc_expr*>& terms) {
    const lc_expr* result = nullptr;
    for (auto it = terms.rbegin(); it != terms.rend(); ++it)
        result = lc.make_app(lc.make_abs(result), *it);
    return result;
}

static std::vector<const lc_expr*> seeded_builtin_terms(lc_expr_pool& lc) {
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
    return {
        dt.transpile_decl(2u, 0u, 0u),
        dt.transpile_decl(2u, 1u, 0u),
        dt.transpile_decl(2u, 0u, 2u),
        dt.transpile_decl(2u, 1u, 0u),
        dt.transpile_decl(2u, 0u, 1u),
        dt.transpile_decl(2u, 1u, 1u),
    };
}

static const lc_expr* outermost_arg(const lc_expr* assembled) {
    return std::get<lc_expr::app>(assembled->content).arg;
}

static const lc_expr* second_arg(const lc_expr* assembled) {
    const auto& outer     = std::get<lc_expr::app>(assembled->content);
    const auto& inner_abs = std::get<lc_expr::abs>(outer.func->content);
    return std::get<lc_expr::app>(inner_abs.body->content).arg;
}

struct ElaboratorManifestIntegrationTest : public ::testing::Test {
    aml_expr_pool aml;
    initial_goal_exprs goals;
    lc_expr_pool  lc;
};

} // namespace

TEST_F(ElaboratorManifestIntegrationTest, ProcessDefinitionSingleIdentity) {
    module_file mod;
    mod.items.push_back(global{definition{"f", aml.make_abs("x", aml.make_symbol("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.global_pump_.pump();

    const lc_expr* result = em.asm_.assemble();

    ASSERT_NE(result, nullptr);
    auto terms = seeded_builtin_terms(lc);
    terms.push_back(lc.make_abs(lc.make_var(0)));
    EXPECT_TRUE(struct_eq(result, assemble_lc(lc, terms)));
}

TEST_F(ElaboratorManifestIntegrationTest, ProcessDefinitionBodyCannotSeeOwnName) {
    module_file mod;
    mod.items.push_back(global{definition{"f", aml.make_abs("x", aml.make_symbol("f"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    EXPECT_THROW(em.global_pump_.pump(), std::out_of_range);
}

TEST_F(ElaboratorManifestIntegrationTest, ProcessDefinitionSecondSeesFirst) {
    module_file mod;
    mod.items.push_back(global{definition{"f", aml.make_abs("x", aml.make_symbol("x"))}});
    mod.items.push_back(global{definition{"g", aml.make_abs("y", aml.make_symbol("f"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.global_pump_.pump();

    const lc_expr* result = em.asm_.assemble();

    ASSERT_NE(result, nullptr);
    auto terms = seeded_builtin_terms(lc);
    terms.push_back(lc.make_abs(lc.make_var(0)));
    terms.push_back(lc.make_abs(lc.make_var(1)));
    EXPECT_TRUE(struct_eq(result, assemble_lc(lc, terms)));
}

TEST_F(ElaboratorManifestIntegrationTest, SeededBuiltinsPresentAfterConstruct) {
    std::vector<module_file> mods;
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    EXPECT_EQ(em.sc.get_var_index(k_nil_name), 2u);
    EXPECT_FALSE(em.globals.empty());
}

TEST_F(ElaboratorManifestIntegrationTest, ProcessDeclarationGroupBooleans) {
    module_file mod;
    mod.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.global_pump_.pump();

    const lc_expr* result = em.asm_.assemble();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(struct_eq(outermost_arg(result),
                          lc.make_abs(lc.make_abs(lc.make_var(1)))));
    EXPECT_TRUE(struct_eq(second_arg(result),
                          lc.make_abs(lc.make_abs(lc.make_var(0)))));
}

TEST_F(ElaboratorManifestIntegrationTest, ProcessStatementsFillTrainingData) {
    const aml_expr* lhs = aml.make_symbol("true");
    const aml_expr* rhs = aml.make_symbol("false");
    statement_file sf;
    sf.statements.push_back({lhs, rhs});

    module_file mod;
    mod.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.global_pump_.pump();
    em.statement_pump_.pump();

    auto dp = em.training.get_next_data_point();
    ASSERT_TRUE(dp.has_value());
    EXPECT_NE(dp->input, nullptr);
    EXPECT_NE(dp->label, nullptr);
    EXPECT_EQ(em.training.get_next_data_point(), std::nullopt);
}
