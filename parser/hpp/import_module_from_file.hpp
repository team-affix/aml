#ifndef IMPORT_MODULE_FROM_FILE_HPP
#define IMPORT_MODULE_FROM_FILE_HPP

#include <fstream>
#include <stdexcept>
#include <string>
#include "../generated/AMLLexer.h"
#include "../generated/AMLParser.h"
#include "../hpp/aml_visitor.hpp"
#include "value_objects/module_file.hpp"

template<typename IMakeAml>
module_file import_module_from_file(const std::string& path, IMakeAml& make_aml) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("cannot open file: " + path);

    antlr4::ANTLRInputStream stream(file);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();

    auto* ctx = parser.moduleFile();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in: " + path);

    aml_visitor<IMakeAml> visitor{make_aml};
    return visitor.parse_module_file(ctx);
}

#endif
