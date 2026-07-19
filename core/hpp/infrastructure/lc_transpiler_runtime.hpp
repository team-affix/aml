#ifndef LC_TRANSPILER_RUNTIME_HPP
#define LC_TRANSPILER_RUNTIME_HPP

#include <vector>
#include "value_objects/lc_expr.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/lc_transpiler_manifest.hpp"

struct lc_transpiler_runtime {
    lc_transpiler_runtime();

    const lc_expr* transpile_definition(const definition& def);

    std::vector<const lc_expr*>
        transpile_declaration_group(const declaration_group& group);

private:
    lc_transpiler_manifest manifest_;
};

#endif
