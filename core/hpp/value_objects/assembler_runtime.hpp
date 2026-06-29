#ifndef ASSEMBLER_RUNTIME_HPP
#define ASSEMBLER_RUNTIME_HPP

#include "value_objects/lc_expr.hpp"
#include "value_objects/assembler_manifest.hpp"

struct assembler_runtime {
    assembler_runtime();

    void           push    (const lc_expr* term);
    const lc_expr* assemble();

private:
    assembler_manifest manifest_;
};

#endif
