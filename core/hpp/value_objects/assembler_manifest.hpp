#ifndef ASSEMBLER_MANIFEST_HPP
#define ASSEMBLER_MANIFEST_HPP

#include "infrastructure/assembler.hpp"
#include "infrastructure/global_stack.hpp"
#include "infrastructure/lc_expr_pool.hpp"

struct assembler_manifest {
    using assembler_t = assembler<lc_expr_pool, lc_expr_pool, global_stack>;

    assembler_manifest();

    lc_expr_pool   lc;
    global_stack   globals;
    assembler_t    asm_;
};

#endif
