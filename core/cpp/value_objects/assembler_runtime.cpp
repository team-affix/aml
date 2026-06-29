#include "value_objects/assembler_runtime.hpp"

assembler_runtime::assembler_runtime() = default;

void assembler_runtime::push(const lc_expr* term) {
    manifest_.globals.push(term);
}

const lc_expr* assembler_runtime::assemble() {
    return manifest_.asm_.assemble();
}
