#ifndef AML_RUNTIME_HPP
#define AML_RUNTIME_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/aml_manifest.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/lc_expr.hpp"

struct aml_runtime {
    aml_runtime();

    void visit_definition(const definition& def);
    void visit_declaration_group(const declaration_group& group);
    const lc_expr* assemble();

private:
    aml_manifest manifest_;
};

#endif
