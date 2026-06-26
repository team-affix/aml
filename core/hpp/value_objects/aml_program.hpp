#ifndef AML_PROGRAM_HPP
#define AML_PROGRAM_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/aml_expr.hpp"

struct constructor_decl {
    std::string name;
    uint32_t    arity;
};

using constructor_group = std::vector<constructor_decl>;

struct function_def {
    std::string     name;
    const aml_expr* body;
};

struct aml_program {
    aml_expr_pool                  pool;
    std::vector<constructor_group> groups;
    std::vector<function_def>      functions;

    aml_program() = default;
    aml_program(const aml_program&) = delete;
    aml_program& operator=(const aml_program&) = delete;
    aml_program(aml_program&&) = default;
    aml_program& operator=(aml_program&&) = default;
};

#endif
