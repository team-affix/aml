#ifndef TRANSPILER_MANIFEST_HPP
#define TRANSPILER_MANIFEST_HPP

#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"

// Composition root for the transpiler pipeline.  Owns all infrastructure
// (expression pool, scope, and every sub-transpiler) and wires them by
// reference.  Mirrors the atlas manifest pattern: pure public data, one
// constructor, no methods.
//
// Member declaration order:
//   tx       — first; receives forward references to sub-components not yet
//              constructed.  References are only USED after full construction,
//              so this is safe (same pattern as atlas manifests).
//   lc, sc   — default-constructed infrastructure.
//   token_ … string_ — leaf sub-transpilers; depend only on lc and sc.
//   abs_ … church_list_ — recursive sub-transpilers; depend on tx (already
//              constructed by the time they are initialized).
struct transpiler_manifest {
    using transpiler_t          = transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool,
                                     scope, scope, scope>;
    using token_transpiler_t       = typename transpiler_t::token_transpiler_t;
    using scott_nat_transpiler_t   = typename transpiler_t::scott_nat_transpiler_t;
    using church_nat_transpiler_t  = typename transpiler_t::church_nat_transpiler_t;
    using integer_transpiler_t     = typename transpiler_t::integer_transpiler_t;
    using character_transpiler_t   = typename transpiler_t::character_transpiler_t;
    using string_transpiler_t      = typename transpiler_t::string_transpiler_t;
    using scott_list_transpiler_t  = typename transpiler_t::scott_list_transpiler_t;
    using church_list_transpiler_t = typename transpiler_t::church_list_transpiler_t;
    using abs_transpiler_t         = typename transpiler_t::abs_transpiler_t;
    using app_transpiler_t         = typename transpiler_t::app_transpiler_t;

    transpiler_manifest();

    // Primary entry points.
    transpiler_t         tx;
    lc_expr_pool lc;
    scope        sc;

    // Non-recursive sub-components.
    token_transpiler_t      token_;
    scott_nat_transpiler_t  scott_nat_;
    church_nat_transpiler_t church_nat_;
    integer_transpiler_t    integer_;
    character_transpiler_t  character_;
    string_transpiler_t     string_;

    // Recursive sub-components (depend on tx).
    abs_transpiler_t        abs_;
    app_transpiler_t        app_;
    scott_list_transpiler_t scott_list_;
    church_list_transpiler_t church_list_;
};

inline transpiler_manifest::transpiler_manifest()
    : tx(token_, abs_, app_,
         scott_nat_, church_nat_,
         integer_, character_,
         scott_list_, church_list_,
         string_),
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

#endif
