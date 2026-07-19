#include <cstdint>
#include "infrastructure/aml_runtime.hpp"

aml_runtime::aml_runtime() = default;

void aml_runtime::visit_definition(const definition& def) {
    const lc_expr* term = manifest_.tx.transpile(def.body);
    manifest_.sc.push(def.name);
    manifest_.globals.push(term);
}

void aml_runtime::visit_declaration_group(const declaration_group& group) {
    auto terms = manifest_.decl_.transpile_group(group);
    for (uint32_t k = 0; k < terms.size(); ++k) {
        manifest_.sc.push(group.declarations.at(k).name);
        manifest_.globals.push(terms[k]);
    }
}

const lc_expr* aml_runtime::assemble() {
    return manifest_.asm_.assemble();
}
