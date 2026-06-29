#ifndef PARSER_RUNTIME_HPP
#define PARSER_RUNTIME_HPP

#include <string>
#include "value_objects/module_file.hpp"
#include "value_objects/statement_file.hpp"
#include "parser_manifest.hpp"

struct parser_runtime {
    parser_runtime();

    module_file    parse_module_file   (const std::string& path);
    statement_file parse_statement_file(const std::string& path);

private:
    parser_manifest manifest_;
};

#endif
