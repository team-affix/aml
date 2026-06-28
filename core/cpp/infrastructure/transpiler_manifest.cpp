#include "infrastructure/transpiler_manifest.hpp"

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
