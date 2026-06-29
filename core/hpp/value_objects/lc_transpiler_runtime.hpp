#ifndef LC_TRANSPILER_RUNTIME_HPP
#define LC_TRANSPILER_RUNTIME_HPP

#include <string>
#include "value_objects/lc_expr.hpp"
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_transpiler_manifest.hpp"

struct lc_transpiler_runtime {
    lc_transpiler_runtime();

    const lc_expr* transpile     (const aml_expr* e);
    void           push_scope    (const std::string& name);
    void           pop_scope     ();

private:
    lc_transpiler_manifest manifest_;
};

#endif
