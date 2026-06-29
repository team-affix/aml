#ifndef ASSEMBLER_MANIFEST_HPP
#define ASSEMBLER_MANIFEST_HPP

#include <stack>
#include "infrastructure/assembler.hpp"
#include "infrastructure/lc_expr_pool.hpp"

struct assembler_manifest {
    using global_stack_t = std::stack<const lc_expr*>;
    using assembler_t    = assembler<lc_expr_pool, lc_expr_pool,
                                     global_stack_t, global_stack_t>;

    assembler_manifest();

    lc_expr_pool   lc;
    global_stack_t globals;
    assembler_t    asm_;
};

#endif
