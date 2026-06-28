#include "value_objects/transpiler_manifest.hpp"

// tx is initialized first (declaration order) and receives forward references
// to the sub-components below it.  Those references are only used after full
// construction, so storing them before the referents are initialized is safe.
// Non-recursive sub-components (token_ … string_) depend only on lc and sc.
// Recursive sub-components (abs_ … church_list_) depend on tx.
transpiler_manifest::transpiler_manifest()
    : tx(token_, abs_, app_,
         scott_nat_, church_nat_,
         integer_, character_,
         string_,
         scott_list_, church_list_),
      token_(lc, sc),
      scott_nat_(lc, lc, sc),
      church_nat_(lc, lc, lc),
      integer_(lc, lc, scott_nat_, sc),
      character_(scott_nat_),
      string_(scott_nat_, lc, lc, sc),
      abs_(tx, lc, sc, sc),
      app_(tx, lc),
      scott_list_(tx, lc, lc, sc),
      church_list_(tx, lc, lc, lc) {}
