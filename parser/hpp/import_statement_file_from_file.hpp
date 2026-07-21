#ifndef IMPORT_STATEMENT_FILE_FROM_FILE_HPP
#define IMPORT_STATEMENT_FILE_FROM_FILE_HPP

#include <fstream>
#include <stdexcept>
#include <string>
#include "../generated/AMLLexer.h"
#include "../generated/AMLParser.h"
#include "../hpp/aml_visitor.hpp"
#include "value_objects/statement_file.hpp"

template<typename IMakeAml>
statement_file import_statement_file_from_file(const std::string& path,
                                               IMakeAml& make_aml) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("cannot open file: " + path);

    antlr4::ANTLRInputStream stream(file);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();

    auto* ctx = parser.statementFile();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in: " + path);

    aml_visitor<IMakeAml> visitor{make_aml};
    return visitor.parse_statement_file(ctx);
}

#endif
