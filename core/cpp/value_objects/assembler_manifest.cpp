#include "value_objects/assembler_manifest.hpp"

// globals is default-initialized (empty); asm_ holds references to lc
// and globals, both of which are fully initialized before asm_ (declaration order).
assembler_manifest::assembler_manifest() : asm_(lc, lc, globals) {}
