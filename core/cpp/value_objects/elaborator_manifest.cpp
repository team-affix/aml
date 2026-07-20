#include "value_objects/elaborator_manifest.hpp"

// tx is initialized first and receives forward references to sub-components.
// Those references are only used after full construction, so storing them
// before the referents are initialized is safe.
// Non-recursive sub-components (token_ … string_) depend only on lc and sc.
// Recursive sub-components (abs_ … church_list_) depend on tx.
// Iterators hold const refs to the caller's file vectors. Processors take refs
// to transpile/push collaborators. Pumps wire iterator → processor.
// Assembler is constructed against lc + globals but not orchestrated yet.
// lc_tx / lc_*_ convert lc_expr* → Atlas expr*; constructed, not orchestrated yet.
// data_point_tx builds normalize(app(M, x), y) goals; not orchestrated yet.
elaborator_manifest::elaborator_manifest(
        const std::vector<module_file>& module_files,
        const std::vector<statement_file>& statement_files)
    : tx(token_, abs_, app_,
         binary_nat_, church_nat_,
         integer_, character_,
         string_,
         scott_list_, church_list_),
      token_(lc, sc),
      binary_nat_(lc, lc, sc),
      church_nat_(lc, lc, lc),
      integer_(lc, lc, binary_nat_, sc),
      character_(binary_nat_),
      string_(binary_nat_, lc, lc, sc),
      abs_(tx, lc, sc, sc),
      app_(tx, lc),
      scott_list_(tx, lc, lc, sc),
      church_list_(tx, lc, lc, lc),
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
      data_point_tx(lc_tx, chc, chc) {}
