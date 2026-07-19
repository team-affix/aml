#include "infrastructure/lc_transpiler_runtime.hpp"

lc_transpiler_runtime::lc_transpiler_runtime() = default;

const lc_expr*
lc_transpiler_runtime::transpile_definition(const definition& def) {
    const lc_expr* term = manifest_.tx.transpile(def.body);
    manifest_.sc.push(def.name);
    return term;
}

std::vector<const lc_expr*>
lc_transpiler_runtime::transpile_declaration_group(const declaration_group& group) {
    auto pairs = manifest_.decl_.transpile_group(group);
    std::vector<const lc_expr*> terms;
    terms.reserve(pairs.size());
    for (auto& [name, term] : pairs) {
        manifest_.sc.push(name);
        terms.push_back(term);
    }
    return terms;
}
