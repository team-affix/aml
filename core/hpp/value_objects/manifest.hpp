#ifndef MANIFEST_HPP
#define MANIFEST_HPP

#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"

struct manifest {
    using transpiler_t             = transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool,
                                        scope, scope, scope>;
    using token_transpiler_t       = typename transpiler_t::token_transpiler_t;
    using abs_transpiler_t         = typename transpiler_t::abs_transpiler_t;
    using app_transpiler_t         = typename transpiler_t::app_transpiler_t;
    using binary_nat_transpiler_t  = typename transpiler_t::binary_nat_transpiler_t;
    using church_nat_transpiler_t  = typename transpiler_t::church_nat_transpiler_t;
    using integer_transpiler_t     = typename transpiler_t::integer_transpiler_t;
    using character_transpiler_t   = typename transpiler_t::character_transpiler_t;
    using string_transpiler_t      = typename transpiler_t::string_transpiler_t;
    using scott_list_transpiler_t  = typename transpiler_t::scott_list_transpiler_t;
    using church_list_transpiler_t = typename transpiler_t::church_list_transpiler_t;

    using global_stack_t = std::stack<const lc_expr*>;
    using assembler_t    = assembler<lc_expr_pool, lc_expr_pool,
                                     global_stack_t, global_stack_t>;

    manifest();

    transpiler_t tx;
    lc_expr_pool lc;
    scope        sc;

    token_transpiler_t      token_;
    binary_nat_transpiler_t binary_nat_;
    church_nat_transpiler_t church_nat_;
    integer_transpiler_t    integer_;
    character_transpiler_t  character_;
    string_transpiler_t     string_;

    abs_transpiler_t         abs_;
    app_transpiler_t         app_;
    scott_list_transpiler_t  scott_list_;
    church_list_transpiler_t church_list_;

    global_stack_t globals;
    assembler_t    asm_;
};

#endif
