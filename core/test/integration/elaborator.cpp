// Integration: elaborator_manifest.elaborator_ fills external initial_goal_exprs.

#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/global.hpp"
#include "value_objects/lc_functor_ids.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

namespace {

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
    const lc_expr* program =
        em.lc.make_app(em.lc.make_abs(nullptr), f_body);
    const expr* P = em.lc_tx.transpile(program);
    const expr* M = em.chc.make_var(k_model_var_id);
    EXPECT_EQ(goals.get(0),
              em.chc.make_functor(k_eq_functor_id, {M, P}));

    const expr* x = em.lc_tx.transpile(em.lc.make_var(0));
    const expr* applied =
        em.chc.make_functor(k_app_functor_id, {M, x});
    EXPECT_EQ(goals.get(1),
              em.chc.make_functor(k_normalize_functor_id, {applied, x}));
}
