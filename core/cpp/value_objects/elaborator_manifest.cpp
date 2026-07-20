#include "value_objects/elaborator_manifest.hpp"

// tx is initialized first and receives forward references to sub-components.
// Those references are only used after full construction, so storing them
// before the referents are initialized is safe.
// Non-recursive sub-components (token_ … string_) depend only on lc and sc.
// Recursive sub-components (abs_ … church_list_) depend on tx.
// Iterators hold const refs to the caller's file vectors. Processors take refs
// to transpile/push collaborators only; a future caller will drive iteration.
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
      statement_proc(tx, training) {}
