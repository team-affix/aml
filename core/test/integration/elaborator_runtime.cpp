// Integration: elaborator_runtime public API only (no manifest_ access).

#include <stdexcept>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/elaborator_runtime.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/global.hpp"
#include "value_objects/lc_functor_ids.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/statement_file.hpp"

namespace {

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = std::vector<declaration>(decls);
    return g;
}

const expr::functor& as_functor(const expr* e) {
    return std::get<expr::functor>(e->content);
}

const expr::var& as_var(const expr* e) {
    return std::get<expr::var>(e->content);
}

uint32_t root_functor_id(const expr* e) {
    return as_functor(e).id;
}

const expr* arg(const expr* e, size_t i) {
    return as_functor(e).args.at(i);
}

// Let-chain depth: count outer app(abs(_), _) wrappers until a non-app (the hole or leaf).
size_t let_chain_depth(const expr* e) {
    size_t n = 0;
    while (std::holds_alternative<expr::functor>(e->content)
           && as_functor(e).id == k_app_functor_id
           && as_functor(e).args.size() == 2
           && std::holds_alternative<expr::functor>(as_functor(e).args.at(0)->content)
           && as_functor(as_functor(e).args.at(0)).id == k_abs_functor_id) {
        e = as_functor(as_functor(e).args.at(0)).args.at(0);
        ++n;
    }
    return n;
}

const expr* let_chain_hole(const expr* e) {
    while (std::holds_alternative<expr::functor>(e->content)
           && as_functor(e).id == k_app_functor_id
           && as_functor(e).args.size() == 2
           && std::holds_alternative<expr::functor>(as_functor(e).args.at(0)->content)
           && as_functor(as_functor(e).args.at(0)).id == k_abs_functor_id)
        e = as_functor(as_functor(e).args.at(0)).args.at(0);
    return e;
}

bool struct_eq_expr(const expr* a, const expr* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->content.index() != b->content.index()) return false;
    if (const auto* va = std::get_if<expr::var>(&a->content))
        return *va == std::get<expr::var>(b->content);
    const auto& fa = std::get<expr::functor>(a->content);
    const auto& fb = std::get<expr::functor>(b->content);
    if (fa.id != fb.id || fa.args.size() != fb.args.size()) return false;
    for (size_t i = 0; i < fa.args.size(); ++i)
        if (!struct_eq_expr(fa.args.at(i), fb.args.at(i))) return false;
    return true;
}

struct ElaboratorRuntimeIntegrationTest : public ::testing::Test {
    aml_expr_pool aml;
};

} // namespace

TEST_F(ElaboratorRuntimeIntegrationTest, SmokeEqThenNormalizeFunctorIds) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    EXPECT_EQ(root_functor_id(goals.get(1)), k_normalize_functor_id);
    EXPECT_EQ(as_functor(goals.get(0)).args.size(), 2u);
    EXPECT_EQ(as_functor(goals.get(1)).args.size(), 2u);
    EXPECT_EQ(as_var(arg(goals.get(0), 0)).index, k_model_var_id);
    EXPECT_EQ(root_functor_id(arg(goals.get(1), 0)), k_app_functor_id);
    EXPECT_EQ(as_var(arg(arg(goals.get(1), 0), 0)).index, k_model_var_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, EmptyProgramIsEqOfModelAndHole) {
    initial_goal_exprs goals;
    std::vector<module_file> mods;
    std::vector<statement_file> stmts;
    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    EXPECT_EQ(as_var(arg(goals.get(0), 0)).index, k_model_var_id);
    EXPECT_EQ(as_var(arg(goals.get(0), 1)).index, k_main_function_var_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, ModulesOnlyYieldsEqOnly) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    EXPECT_EQ(let_chain_depth(arg(goals.get(0), 1)), 1u);
    EXPECT_EQ(as_var(let_chain_hole(arg(goals.get(0), 1))).index,
              k_main_function_var_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, UnboundStatementThrowsGoalsEmpty) {
    initial_goal_exprs goals;
    statement_file sf;
    sf.statements.push_back({aml.make_token("missing"), aml.make_token("missing")});
    std::vector<module_file> mods;
    std::vector<statement_file> stmts{sf};
    elaborator_runtime rt{mods, stmts, goals};
    EXPECT_THROW(rt.elaborate(), std::out_of_range);
    EXPECT_EQ(goals.count(), 0u);
}

TEST_F(ElaboratorRuntimeIntegrationTest, SelfReferentialDefinitionThrowsGoalsEmpty) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("f"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_runtime rt{mods, stmts, goals};
    EXPECT_THROW(rt.elaborate(), std::out_of_range);
    EXPECT_EQ(goals.count(), 0u);
}

TEST_F(ElaboratorRuntimeIntegrationTest, MultiStatementNormalizeOrder) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf;
    const aml_expr* id = aml.make_abs("x", aml.make_token("x"));
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    sf.statements.push_back({aml.make_token("f"), id});
    sf.statements.push_back({id, aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 4u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    for (size_t i = 1; i < 4; ++i)
        EXPECT_EQ(root_functor_id(goals.get(i)), k_normalize_functor_id);

    // Distinct normalize payloads (f≈f vs f≈λx.x vs λx.x≈f).
    EXPECT_TRUE(struct_eq_expr(arg(goals.get(1), 0), arg(goals.get(1), 0)));
    EXPECT_FALSE(struct_eq_expr(arg(goals.get(1), 1), arg(goals.get(2), 1)));
    EXPECT_FALSE(struct_eq_expr(goals.get(2), goals.get(3)));
}

TEST_F(ElaboratorRuntimeIntegrationTest, TwoStatementFilesConcatenatedInOrder) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf0;
    sf0.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    statement_file sf1;
    const aml_expr* id = aml.make_abs("x", aml.make_token("x"));
    sf1.statements.push_back({aml.make_token("f"), id});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf0, sf1};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 3u);
    EXPECT_EQ(root_functor_id(goals.get(1)), k_normalize_functor_id);
    EXPECT_EQ(root_functor_id(goals.get(2)), k_normalize_functor_id);
    EXPECT_FALSE(struct_eq_expr(goals.get(1), goals.get(2)));
}

TEST_F(ElaboratorRuntimeIntegrationTest, IdentityProgramStructuralShape) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    const expr* P = arg(goals.get(0), 1);
    EXPECT_EQ(let_chain_depth(P), 1u);
    EXPECT_EQ(as_var(let_chain_hole(P)).index, k_main_function_var_id);

    const expr* norm = goals.get(1);
    EXPECT_EQ(root_functor_id(norm), k_normalize_functor_id);
    const expr* applied = arg(norm, 0);
    EXPECT_EQ(root_functor_id(applied), k_app_functor_id);
    EXPECT_EQ(as_var(arg(applied, 0)).index, k_model_var_id);
    // var(zero) for both sides of f≈f
    EXPECT_EQ(root_functor_id(arg(applied, 1)), k_var_functor_id);
    EXPECT_EQ(root_functor_id(arg(arg(applied, 1), 0)), k_zero_functor_id);
    EXPECT_EQ(root_functor_id(arg(norm, 1)), k_var_functor_id);
    EXPECT_EQ(root_functor_id(arg(arg(norm, 1), 0)), k_zero_functor_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, ThreeGlobalsLetChainDepth) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{
        definition{"a", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{
        definition{"b", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{
        definition{"c", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    const expr* P = arg(goals.get(0), 1);
    EXPECT_EQ(let_chain_depth(P), 3u);
    EXPECT_EQ(as_var(let_chain_hole(P)).index, k_main_function_var_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, BooleanNormalizeSidesDiffer) {
    initial_goal_exprs goals;
    module_file mod;
    mod.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    statement_file sf;
    sf.statements.push_back({aml.make_token("true"), aml.make_token("false")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const expr* applied = arg(goals.get(1), 0);
    EXPECT_FALSE(struct_eq_expr(arg(applied, 1), arg(goals.get(1), 1)));
}

TEST_F(ElaboratorRuntimeIntegrationTest, TwoRuntimesIndependentStructurallyEqual) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    initial_goal_exprs g1;
    initial_goal_exprs g2;
    elaborator_runtime r1{mods, stmts, g1};
    elaborator_runtime r2{mods, stmts, g2};
    r1.elaborate();
    r2.elaborate();

    ASSERT_EQ(g1.count(), 2u);
    ASSERT_EQ(g2.count(), 2u);
    EXPECT_TRUE(struct_eq_expr(g1.get(0), g2.get(0)));
    EXPECT_TRUE(struct_eq_expr(g1.get(1), g2.get(1)));
    EXPECT_NE(g1.get(0), g2.get(0));
}

TEST_F(ElaboratorRuntimeIntegrationTest, SharedGoalsAppendAcrossRuntimes) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;

    initial_goal_exprs goals;
    elaborator_runtime r1{mods, stmts, goals};
    r1.elaborate();
    EXPECT_EQ(goals.count(), 1u);

    elaborator_runtime r2{mods, stmts, goals};
    r2.elaborate();
    EXPECT_EQ(goals.count(), 2u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    EXPECT_EQ(root_functor_id(goals.get(1)), k_eq_functor_id);
}

TEST_F(ElaboratorRuntimeIntegrationTest, ZeroGlobalsChurchNatStatement) {
    initial_goal_exprs goals;
    statement_file sf;
    sf.statements.push_back({
        aml.make_nat(0u, nat_format::church),
        aml.make_nat(1u, nat_format::church)});
    std::vector<module_file> mods;
    std::vector<statement_file> stmts{sf};

    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    EXPECT_EQ(as_var(arg(goals.get(0), 1)).index, k_main_function_var_id);
    EXPECT_EQ(root_functor_id(goals.get(1)), k_normalize_functor_id);
    EXPECT_FALSE(struct_eq_expr(arg(arg(goals.get(1), 0), 1), arg(goals.get(1), 1)));
}

TEST_F(ElaboratorRuntimeIntegrationTest, YCombinatorWithTrainingData) {
    // id ; Y = λf. (λx. f (x x)) (λx. f (x x)) ; Y id ≈ id ; Y ≈ Y
    const aml_expr* omega = aml.make_abs("x",
        aml.make_app(aml.make_token("f"),
            aml.make_app(aml.make_token("x"), aml.make_token("x"))));
    const aml_expr* Y = aml.make_abs("f", aml.make_app(omega, omega));

    module_file mod;
    mod.items.push_back(global{
        definition{"id", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{definition{"Y", Y}});
    statement_file sf;
    sf.statements.push_back({
        aml.make_app(aml.make_token("Y"), aml.make_token("id")),
        aml.make_token("id")});
    sf.statements.push_back({aml.make_token("Y"), aml.make_token("Y")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};

    initial_goal_exprs goals;
    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    ASSERT_EQ(goals.count(), 3u);
    EXPECT_EQ(root_functor_id(goals.get(0)), k_eq_functor_id);
    EXPECT_EQ(let_chain_depth(arg(goals.get(0), 1)), 2u);
    EXPECT_EQ(as_var(let_chain_hole(arg(goals.get(0), 1))).index,
              k_main_function_var_id);
    EXPECT_EQ(root_functor_id(goals.get(1)), k_normalize_functor_id);
    EXPECT_EQ(root_functor_id(goals.get(2)), k_normalize_functor_id);
    // Y id ≈ id  vs  Y ≈ Y — distinct normalize goals
    EXPECT_FALSE(struct_eq_expr(goals.get(1), goals.get(2)));
    EXPECT_EQ(root_functor_id(arg(goals.get(1), 0)), k_app_functor_id);
    EXPECT_EQ(as_var(arg(arg(goals.get(1), 0), 0)).index, k_model_var_id);
}
