#include "value_objects/elaborator_manifest.hpp"

// tx is initialized first and receives forward references to sub-components.
// Those references are only used after full construction, so storing them
// before the referents are initialized is safe.
// decl / builtin_ are constructed before sugar/symbol transpilers that use them.
// Non-recursive sub-components (symbol_ … string_) depend on lc, sc, builtin_.
// Recursive sub-components (abs_ … list_) depend on tx.
// Iterators hold const refs to the caller's file vectors. Processors take refs
// to transpile/push collaborators. Pumps wire iterator → processor.
// Assembler drains globals LIFO into a let-chain with nullptr innermost body.
// lc_tx / lc_*_ convert lc_expr* → Atlas expr*.
// data_point_tx builds normalize(app(M, x), y) goals.
// elaborator_ orchestrates pumps → assemble → transpile → push initial goals.
// Builtin Scott ctors are encoded in-place via builtin_ (no seeded globals).
elaborator_manifest::elaborator_manifest(
        const std::vector<module_file>& module_files,
        const std::vector<statement_file>& statement_files,
        initial_goal_exprs& initial_goals)
    : decl(lc, lc, lc)
    , builtin_(decl)
    , tx(symbol_, abs_, app_,
         nat_,
         integer_, character_,
         string_,
         list_),
      symbol_(lc, sc, sc, builtin_),
      nat_(lc, lc, lc, builtin_, builtin_, builtin_, builtin_),
      integer_(lc, nat_, builtin_, builtin_),
      character_(nat_),
      string_(nat_, lc, builtin_, builtin_),
      abs_(tx, lc, sc, sc),
      app_(tx, lc),
      list_(tx, lc, lc, lc, builtin_, builtin_),
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
                  data_point_tx, chc, chc, initial_goals) {}
