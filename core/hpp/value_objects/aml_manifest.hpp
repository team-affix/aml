#ifndef AML_MANIFEST_HPP
#define AML_MANIFEST_HPP

#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/transpiler.hpp"

struct aml_manifest {
    using global_stack_t       = std::stack<const lc_expr*>;
    using transpiler_t         = transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool,
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
    using declaration_transpiler_t = declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool>;
    using assembler_t              = assembler<lc_expr_pool, lc_expr_pool,
                                               global_stack_t, global_stack_t>;

    aml_manifest();

    transpiler_t   tx;
    lc_expr_pool   lc;
    scope          sc;
    global_stack_t globals;
    assembler_t    asm_;

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
    declaration_transpiler_t decl_;
};

#endif
