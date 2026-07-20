#ifndef ELABORATOR_MANIFEST_HPP
#define ELABORATOR_MANIFEST_HPP

#include <vector>
#include "infrastructure/assembler.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/global_iterator.hpp"
#include "infrastructure/global_processor.hpp"
#include "infrastructure/global_pump.hpp"
#include "infrastructure/global_stack.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/lc_expr_transpiler.hpp"
#include "infrastructure/scope.hpp"
#include "infrastructure/statement_iterator.hpp"
#include "infrastructure/statement_processor.hpp"
#include "infrastructure/statement_pump.hpp"
#include "infrastructure/training_data.hpp"
#include "infrastructure/transpiler.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement_file.hpp"

struct elaborator_manifest {
    using transpiler_t   = transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool,
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
    using global_processor_t       = global_processor<transpiler_t, declaration_transpiler_t,
                                                      global_stack, scope>;
    using statement_processor_t    = statement_processor<transpiler_t, training_data>;
    using global_pump_t            = global_pump<global_iterator, global_processor_t>;
    using statement_pump_t         = statement_pump<statement_iterator, statement_processor_t>;
    using assembler_t              = assembler<lc_expr_pool, lc_expr_pool, global_stack>;

    using lc_tx_t                  = lc_expr_transpiler<expr_pool, expr_pool>;
    using lc_nullptr_transpiler_t  = typename lc_tx_t::lc_nullptr_transpiler_t;
    using lc_var_transpiler_t      = typename lc_tx_t::lc_var_transpiler_t;
    using lc_abs_transpiler_t      = typename lc_tx_t::lc_abs_transpiler_t;
    using lc_app_transpiler_t      = typename lc_tx_t::lc_app_transpiler_t;

    elaborator_manifest(const std::vector<module_file>& module_files,
                        const std::vector<statement_file>& statement_files);

    lc_expr_pool          lc;
    scope                 sc;
    global_stack          globals;
    training_data         training;
    expr_pool             chc;

    transpiler_t              tx;
    token_transpiler_t        token_;
    binary_nat_transpiler_t   binary_nat_;
    church_nat_transpiler_t   church_nat_;
    integer_transpiler_t      integer_;
    character_transpiler_t    character_;
    string_transpiler_t       string_;
    abs_transpiler_t          abs_;
    app_transpiler_t          app_;
    scott_list_transpiler_t   scott_list_;
    church_list_transpiler_t  church_list_;
    declaration_transpiler_t  decl;

    global_iterator       global_it;
    statement_iterator    statement_it;
    global_processor_t    global_proc;
    statement_processor_t statement_proc;
    global_pump_t         global_pump_;
    statement_pump_t      statement_pump_;
    assembler_t           asm_;

    lc_tx_t                   lc_tx;
    lc_var_transpiler_t       lc_var_;
    lc_abs_transpiler_t       lc_abs_;
    lc_app_transpiler_t       lc_app_;
    lc_nullptr_transpiler_t   lc_nullptr_;
};

#endif
