#include "value_objects/aml_manifest.hpp"

// tx is initialized first and receives forward references to sub-components.
// Those references are only used after full construction, so storing them
// before the referents are initialized is safe.
// Non-recursive sub-components (token_ … string_) depend only on lc and sc.
// Recursive sub-components (abs_ … church_list_) depend on tx.
// asm_ depends on lc and globals, both of which are declared before it.
aml_manifest::aml_manifest()
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
      asm_(lc, lc, globals, globals),
      abs_(tx, lc, sc, sc),
      app_(tx, lc),
      scott_list_(tx, lc, lc, sc),
      church_list_(tx, lc, lc, lc),
      decl_(lc, lc, lc) {}
