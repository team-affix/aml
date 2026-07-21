#include "value_objects/elaborator_manifest.hpp"

#include "value_objects/bool_decl_group.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/global.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/int_decl_group.hpp"

namespace {

void seed_builtin_declarations(elaborator_manifest::global_processor_t& global_proc) {
    global_proc.process_global(global{declaration_group{{
        {k_true_name, k_true_arity},
        {k_false_name, k_false_arity}}}});
    global_proc.process_global(global{declaration_group{{
        {k_cons_name, k_cons_arity},
        {k_nil_name, k_nil_arity}}}});
    global_proc.process_global(global{declaration_group{{
        {k_pos_name, k_pos_arity},
        {k_negsuc_name, k_negsuc_arity}}}});
}

} // namespace

// tx is initialized first and receives forward references to sub-components.
// Those references are only used after full construction, so storing them
// before the referents are initialized is safe.
// Non-recursive sub-components (symbol_ … string_) depend only on lc and sc.
// Recursive sub-components (abs_ … list_) depend on tx.
// Iterators hold const refs to the caller's file vectors. Processors take refs
// to transpile/push collaborators. Pumps wire iterator → processor.
// Assembler drains globals LIFO into a let-chain with nullptr innermost body.
// lc_tx / lc_*_ convert lc_expr* → Atlas expr*.
// data_point_tx builds normalize(app(M, x), y) goals.
// elaborator_ orchestrates pumps → assemble → transpile → push initial goals.
// Builtin Scott groups are seeded into scope + globals after wiring so
// get_var_index and assemble stay in lockstep.
elaborator_manifest::elaborator_manifest(
        const std::vector<module_file>& module_files,
        const std::vector<statement_file>& statement_files,
        initial_goal_exprs& initial_goals)
    : tx(symbol_, abs_, app_,
         nat_,
         integer_, character_,
         string_,
         list_),
      symbol_(lc, sc),
      nat_(lc, lc, lc, sc),
      integer_(lc, lc, nat_, sc),
      character_(nat_),
      string_(nat_, lc, lc, sc),
      abs_(tx, lc, sc, sc),
      app_(tx, lc),
      list_(tx, lc, lc, lc, sc),
      decl(lc, lc, lc),
      global_it(module_files),
      statement_it(statement_files),
      global_proc(tx, decl, globals, sc),
      statement_proc(tx, training),
      global_pump_(global_it, global_proc),
      statement_pump_(statement_it, statement_proc),
      asm_(lc, lc, globals),
      lc_tx(lc_var_, lc_abs_, lc_app_, lc_nullptr_),
      lc_var_(chc),
      lc_abs_(lc_tx, chc),
      lc_app_(lc_tx, chc),
      lc_nullptr_(chc),
      data_point_tx(lc_tx, chc, chc),
      elaborator_(global_pump_, statement_pump_, asm_, lc_tx, training,
                  data_point_tx, chc, chc, initial_goals) {
    seed_builtin_declarations(global_proc);
}
