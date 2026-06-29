#include "value_objects/lc_transpiler_runtime.hpp"

lc_transpiler_runtime::lc_transpiler_runtime() = default;

const lc_expr* lc_transpiler_runtime::transpile(const aml_expr* e) {
    return manifest_.tx.transpile(e);
}

void lc_transpiler_runtime::push_scope(const std::string& name) {
    manifest_.sc.push(name);
}

void lc_transpiler_runtime::pop_scope() {
    manifest_.sc.pop();
}
