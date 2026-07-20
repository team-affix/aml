// Integration: elaborator_manifest.elaborator_ fills external initial_goal_exprs.
// In-memory AML modules/statements; pointer-eq against hand-built terms from em.chc.

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/global.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/nat_format.hpp"
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

namespace {

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = std::vector<declaration>(decls);
    return g;
}

// terms in definition order (first-defined first) → assembled let-chain with nullptr hole.
const lc_expr* assemble_lc(lc_expr_pool& lc,
                           const std::vector<const lc_expr*>& terms_first_to_last) {
    const lc_expr* result = nullptr;
    for (auto it = terms_first_to_last.rbegin();
         it != terms_first_to_last.rend(); ++it)
        result = lc.make_app(lc.make_abs(result), *it);
    return result;
}

const expr* expect_eq(elaborator_manifest& em, const lc_expr* program) {
    const expr* P = em.lc_tx.transpile(program);
    const expr* M = em.chc.make_var(k_model_var_id);
    return em.chc.make_functor(k_eq_functor_id, {M, P});
}

const expr* expect_normalize(elaborator_manifest& em,
                             const lc_expr* input,
                             const lc_expr* label) {
    const expr* M = em.chc.make_var(k_model_var_id);
    const expr* x = em.lc_tx.transpile(input);
    const expr* y = em.lc_tx.transpile(label);
    const expr* applied = em.chc.make_functor(k_app_functor_id, {M, x});
    return em.chc.make_functor(k_normalize_functor_id, {applied, y});
}

// Y = λf. (λx. f (x x)) (λx. f (x x))
const aml_expr* make_y_aml(aml_expr_pool& aml) {
    const aml_expr* omega = aml.make_abs("x",
        aml.make_app(aml.make_token("f"),
            aml.make_app(aml.make_token("x"), aml.make_token("x"))));
    return aml.make_abs("f", aml.make_app(omega, omega));
}

const lc_expr* make_y_lc(lc_expr_pool& lc) {
    const lc_expr* omega = lc.make_abs(
        lc.make_app(lc.make_var(1),
            lc.make_app(lc.make_var(0), lc.make_var(0))));
    return lc.make_abs(lc.make_app(omega, omega));
}

struct ElaboratorIntegrationTest : public ::testing::Test {
    aml_expr_pool      aml;
    initial_goal_exprs goals;
};

} // namespace

TEST_F(ElaboratorIntegrationTest, ElaboratePushesEqThenNormalizeGoals) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});

    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};

    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);

    const lc_expr* f_body = em.lc.make_abs(em.lc.make_var(0));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f_body})));
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, EmptyModulesEmptyStatementsEqHoleOnly) {
    std::vector<module_file> mods;
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    EXPECT_EQ(goals.get(0), expect_eq(em, nullptr));
    EXPECT_TRUE(em.globals.empty());
    EXPECT_EQ(em.training.get_next_data_point(), std::nullopt);
}

TEST_F(ElaboratorIntegrationTest, ModulesOnlyEqAndDrainsStackAndTraining) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    const lc_expr* f = em.lc.make_abs(em.lc.make_var(0));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f})));
    EXPECT_TRUE(em.globals.empty());
    EXPECT_EQ(em.training.get_next_data_point(), std::nullopt);
}

TEST_F(ElaboratorIntegrationTest, UnboundStatementThrowsBeforeAnyGoal) {
    statement_file sf;
    sf.statements.push_back({aml.make_token("x"), aml.make_token("x")});
    std::vector<module_file> mods;
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    EXPECT_THROW(em.elaborator_.elaborate(), std::out_of_range);
    EXPECT_EQ(goals.count(), 0u);
}

TEST_F(ElaboratorIntegrationTest, SelfAppDefinitionAndStatement) {
    // f = λx. x x ; f ≈ f
    module_file mod;
    mod.items.push_back(global{definition{"f",
        aml.make_abs("x",
            aml.make_app(aml.make_token("x"), aml.make_token("x")))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* f = em.lc.make_abs(
        em.lc.make_app(em.lc.make_var(0), em.lc.make_var(0)));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f})));
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, KCombinatorDefinition) {
    // f = λx. λy. x
    module_file mod;
    mod.items.push_back(global{definition{"f",
        aml.make_abs("x", aml.make_abs("y", aml.make_token("x")))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* f = em.lc.make_abs(em.lc.make_abs(em.lc.make_var(1)));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f})));
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, TwoDefsIdThenConstAssembleOrder) {
    module_file mod;
    mod.items.push_back(global{
        definition{"id", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{definition{"const",
        aml.make_abs("x", aml.make_abs("y", aml.make_token("x")))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 1u);
    const lc_expr* id    = em.lc.make_abs(em.lc.make_var(0));
    const lc_expr* konst = em.lc.make_abs(em.lc.make_abs(em.lc.make_var(1)));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {id, konst})));
}

TEST_F(ElaboratorIntegrationTest, TwoDefsFThenGReferencingF) {
    // f = λx.x ; g = λx. f x ; g ≈ f
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{definition{"g",
        aml.make_abs("x",
            aml.make_app(aml.make_token("f"), aml.make_token("x")))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("g"), aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* f = em.lc.make_abs(em.lc.make_var(0));
    const lc_expr* g = em.lc.make_abs(
        em.lc.make_app(em.lc.make_var(1), em.lc.make_var(0)));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f, g})));
    // statement scope: f, g → g=0, f=1
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(1)));
}

TEST_F(ElaboratorIntegrationTest, ThreeStatementsPreserveOrder) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    const aml_expr* id = aml.make_abs("x", aml.make_token("x"));
    statement_file sf;
    sf.statements.push_back({aml.make_token("f"), aml.make_token("f")});
    sf.statements.push_back({aml.make_token("f"), id});
    sf.statements.push_back({id, aml.make_token("f")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 4u);
    const lc_expr* f = em.lc.make_abs(em.lc.make_var(0));
    const lc_expr* id_lc = em.lc.make_abs(em.lc.make_var(0));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {f})));
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
    EXPECT_EQ(goals.get(2),
              expect_normalize(em, em.lc.make_var(0), id_lc));
    EXPECT_EQ(goals.get(3),
              expect_normalize(em, id_lc, em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, NotTrueFalseEndToEnd) {
    // true/false decls + not = λb. b false true ; not true ≈ false
    module_file mod;
    mod.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    mod.items.push_back(global{definition{"not",
        aml.make_abs("b",
            aml.make_app(
                aml.make_app(aml.make_token("b"), aml.make_token("false")),
                aml.make_token("true")))}});
    statement_file sf;
    sf.statements.push_back({
        aml.make_app(aml.make_token("not"), aml.make_token("true")),
        aml.make_token("false")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* true_t  = em.decl.transpile_decl(2u, 0u, 0u);
    const lc_expr* false_t = em.decl.transpile_decl(2u, 1u, 0u);
    // not body under scope true,false then b: b=0, false=1, true=2
    const lc_expr* not_t = em.lc.make_abs(
        em.lc.make_app(
            em.lc.make_app(em.lc.make_var(0), em.lc.make_var(1)),
            em.lc.make_var(2)));
    EXPECT_EQ(goals.get(0),
              expect_eq(em, assemble_lc(em.lc, {true_t, false_t, not_t})));

    // statement scope: true, false, not → not=0, false=1, true=2
    const lc_expr* lhs = em.lc.make_app(em.lc.make_var(0), em.lc.make_var(2));
    const lc_expr* rhs = em.lc.make_var(1);
    EXPECT_EQ(goals.get(1), expect_normalize(em, lhs, rhs));

    const expr* M = em.chc.make_var(k_model_var_id);
    EXPECT_EQ(std::get<expr::functor>(goals.get(0)->content).args.at(0), M);
    const auto& norm = std::get<expr::functor>(goals.get(1)->content);
    const auto& app  = std::get<expr::functor>(norm.args.at(0)->content);
    EXPECT_EQ(app.args.at(0), M);
}

TEST_F(ElaboratorIntegrationTest, FiveDefsOldestNewestStatementIndices) {
    module_file mod;
    for (int i = 0; i < 5; ++i) {
        mod.items.push_back(global{definition{
            std::string(1, static_cast<char>('a' + i)),
            aml.make_abs("x", aml.make_token("x"))}});
    }
    statement_file sf;
    sf.statements.push_back({aml.make_token("a"), aml.make_token("e")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    std::vector<const lc_expr*> terms;
    for (int i = 0; i < 5; ++i)
        terms.push_back(em.lc.make_abs(em.lc.make_var(0)));
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, terms)));
    // a=4 (oldest), e=0 (newest)
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(4), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, TwoModuleFilesDeclsThenDefs) {
    module_file mod0;
    mod0.items.push_back(global{make_group({{"true", 0u}, {"false", 0u}})});
    module_file mod1;
    mod1.items.push_back(global{definition{"not",
        aml.make_abs("b",
            aml.make_app(
                aml.make_app(aml.make_token("b"), aml.make_token("false")),
                aml.make_token("true")))}});
    statement_file sf;
    sf.statements.push_back({
        aml.make_app(aml.make_token("not"), aml.make_token("false")),
        aml.make_token("true")});
    std::vector<module_file> mods{mod0, mod1};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* true_t  = em.decl.transpile_decl(2u, 0u, 0u);
    const lc_expr* false_t = em.decl.transpile_decl(2u, 1u, 0u);
    const lc_expr* not_t = em.lc.make_abs(
        em.lc.make_app(
            em.lc.make_app(em.lc.make_var(0), em.lc.make_var(1)),
            em.lc.make_var(2)));
    EXPECT_EQ(goals.get(0),
              expect_eq(em, assemble_lc(em.lc, {true_t, false_t, not_t})));
    // not false ≈ true → app(var(0), var(1)) ≈ var(2)
    EXPECT_EQ(goals.get(1),
              expect_normalize(em,
                  em.lc.make_app(em.lc.make_var(0), em.lc.make_var(1)),
                  em.lc.make_var(2)));
}

TEST_F(ElaboratorIntegrationTest, ZeroGlobalsChurchNatDataPoint) {
    statement_file sf;
    sf.statements.push_back({
        aml.make_nat(0u, nat_format::church),
        aml.make_nat(2u, nat_format::church)});
    std::vector<module_file> mods;
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    EXPECT_EQ(goals.get(0), expect_eq(em, nullptr));
    const lc_expr* c0 = em.lc.make_abs(em.lc.make_abs(em.lc.make_var(0)));
    const lc_expr* c2 = em.lc.make_abs(em.lc.make_abs(
        em.lc.make_app(em.lc.make_var(1),
            em.lc.make_app(em.lc.make_var(1), em.lc.make_var(0)))));
    EXPECT_EQ(goals.get(1), expect_normalize(em, c0, c2));
}

TEST_F(ElaboratorIntegrationTest, SharedGoalsAppendAcrossManifests) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;

    elaborator_manifest em1{mods, stmts, goals};
    em1.elaborator_.elaborate();
    EXPECT_EQ(goals.count(), 1u);

    elaborator_manifest em2{mods, stmts, goals};
    em2.elaborator_.elaborate();
    EXPECT_EQ(goals.count(), 2u);
}

TEST_F(ElaboratorIntegrationTest, SeparateGoalsAreIndependent) {
    module_file mod;
    mod.items.push_back(global{
        definition{"f", aml.make_abs("x", aml.make_token("x"))}});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts;

    initial_goal_exprs g2;
    elaborator_manifest em1{mods, stmts, goals};
    elaborator_manifest em2{mods, stmts, g2};
    em1.elaborator_.elaborate();
    em2.elaborator_.elaborate();
    EXPECT_EQ(goals.count(), 1u);
    EXPECT_EQ(g2.count(), 1u);
    EXPECT_NE(goals.get(0), g2.get(0));
}

TEST_F(ElaboratorIntegrationTest, ManyGlobalsDeclsDefsAndFourDataPoints) {
    // true,false,cons,nil + id + not ; 4 statements
    module_file mod;
    mod.items.push_back(global{make_group({
        {"true", 0u}, {"false", 0u}, {"cons", 2u}, {"nil", 0u}})});
    mod.items.push_back(global{
        definition{"id", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{definition{"not",
        aml.make_abs("b",
            aml.make_app(
                aml.make_app(aml.make_token("b"), aml.make_token("false")),
                aml.make_token("true")))}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("true"), aml.make_token("true")});
    sf.statements.push_back({aml.make_token("false"), aml.make_token("false")});
    sf.statements.push_back({aml.make_token("id"), aml.make_token("id")});
    sf.statements.push_back({
        aml.make_app(aml.make_token("not"), aml.make_token("true")),
        aml.make_token("false")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 5u);

    const lc_expr* true_t  = em.decl.transpile_decl(4u, 0u, 0u);
    const lc_expr* false_t = em.decl.transpile_decl(4u, 1u, 0u);
    const lc_expr* cons_t  = em.decl.transpile_decl(4u, 2u, 2u);
    const lc_expr* nil_t   = em.decl.transpile_decl(4u, 3u, 0u);
    const lc_expr* id_t    = em.lc.make_abs(em.lc.make_var(0));
    // scope before not body: true,false,cons,nil,id (5); push b →
    // b=0, id=1, nil=2, cons=3, false=4, true=5
    const lc_expr* not_t = em.lc.make_abs(
        em.lc.make_app(
            em.lc.make_app(em.lc.make_var(0), em.lc.make_var(4)),
            em.lc.make_var(5)));
    EXPECT_EQ(goals.get(0),
              expect_eq(em, assemble_lc(em.lc, {
                  true_t, false_t, cons_t, nil_t, id_t, not_t})));

    // statement scope: true,false,cons,nil,id,not (6)
    // true=5, false=4, cons=3, nil=2, id=1, not=0
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(5), em.lc.make_var(5)));
    EXPECT_EQ(goals.get(2),
              expect_normalize(em, em.lc.make_var(4), em.lc.make_var(4)));
    EXPECT_EQ(goals.get(3),
              expect_normalize(em, em.lc.make_var(1), em.lc.make_var(1)));
    EXPECT_EQ(goals.get(4),
              expect_normalize(em,
                  em.lc.make_app(em.lc.make_var(0), em.lc.make_var(5)),
                  em.lc.make_var(4)));
}

TEST_F(ElaboratorIntegrationTest, YCombinatorDefinitionAndSelfStatement) {
    // Y = λf. (λx. f (x x)) (λx. f (x x)) ; Y ≈ Y
    module_file mod;
    mod.items.push_back(global{definition{"Y", make_y_aml(aml)}});
    statement_file sf;
    sf.statements.push_back({aml.make_token("Y"), aml.make_token("Y")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 2u);
    const lc_expr* Y = make_y_lc(em.lc);
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {Y})));
    EXPECT_EQ(goals.get(1),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, YCombinatorAppliedToIdInTrainingData) {
    // id = λx.x ; Y = … ; training: Y id ≈ id  and  Y ≈ Y
    module_file mod;
    mod.items.push_back(global{
        definition{"id", aml.make_abs("x", aml.make_token("x"))}});
    mod.items.push_back(global{definition{"Y", make_y_aml(aml)}});
    statement_file sf;
    sf.statements.push_back({
        aml.make_app(aml.make_token("Y"), aml.make_token("id")),
        aml.make_token("id")});
    sf.statements.push_back({aml.make_token("Y"), aml.make_token("Y")});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 3u);
    const lc_expr* id = em.lc.make_abs(em.lc.make_var(0));
    const lc_expr* Y  = make_y_lc(em.lc);
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {id, Y})));

    // statement scope: id, Y → Y=0, id=1
    EXPECT_EQ(goals.get(1),
              expect_normalize(em,
                  em.lc.make_app(em.lc.make_var(0), em.lc.make_var(1)),
                  em.lc.make_var(1)));
    EXPECT_EQ(goals.get(2),
              expect_normalize(em, em.lc.make_var(0), em.lc.make_var(0)));
}

TEST_F(ElaboratorIntegrationTest, YCombinatorWithConstAndChurchData) {
    // const = λx.λy.x ; Y ; training uses Y const and church nats as labels
    module_file mod;
    mod.items.push_back(global{definition{"const",
        aml.make_abs("x", aml.make_abs("y", aml.make_token("x")))}});
    mod.items.push_back(global{definition{"Y", make_y_aml(aml)}});
    statement_file sf;
    // Y const ≈ const  (fixed point of K is K, syntactically as a training pair)
    sf.statements.push_back({
        aml.make_app(aml.make_token("Y"), aml.make_token("const")),
        aml.make_token("const")});
    // Y (λf. <church> 0) ≈ <church> 0
    const aml_expr* ignore_f =
        aml.make_abs("f", aml.make_nat(0u, nat_format::church));
    sf.statements.push_back({
        aml.make_app(aml.make_token("Y"), ignore_f),
        aml.make_nat(0u, nat_format::church)});
    std::vector<module_file> mods{mod};
    std::vector<statement_file> stmts{sf};
    elaborator_manifest em{mods, stmts, goals};
    em.elaborator_.elaborate();

    ASSERT_EQ(goals.count(), 3u);
    const lc_expr* konst = em.lc.make_abs(em.lc.make_abs(em.lc.make_var(1)));
    const lc_expr* Y     = make_y_lc(em.lc);
    EXPECT_EQ(goals.get(0), expect_eq(em, assemble_lc(em.lc, {konst, Y})));

    // scope: const, Y → Y=0, const=1
    EXPECT_EQ(goals.get(1),
              expect_normalize(em,
                  em.lc.make_app(em.lc.make_var(0), em.lc.make_var(1)),
                  em.lc.make_var(1)));

    const lc_expr* c0 = em.lc.make_abs(em.lc.make_abs(em.lc.make_var(0)));
    // λf. c0 under statement scope (no extra binders beyond f): abs(c0)
    // f is binder; c0 is closed → abs(c0) with c0 = abs(abs(var(0)))
    const lc_expr* ignore_f_lc = em.lc.make_abs(c0);
    EXPECT_EQ(goals.get(2),
              expect_normalize(em,
                  em.lc.make_app(em.lc.make_var(0), ignore_f_lc),
                  c0));
}
