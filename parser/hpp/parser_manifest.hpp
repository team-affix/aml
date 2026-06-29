#ifndef PARSER_MANIFEST_HPP
#define PARSER_MANIFEST_HPP

#include "infrastructure/aml_expr_pool.hpp"
#include "aml_visitor.hpp"

struct parser_manifest {
    parser_manifest();

    aml_expr_pool              aml_;
    aml_visitor<aml_expr_pool> visitor_;
};

#endif
