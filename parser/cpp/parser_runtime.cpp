#include <fstream>
#include <stdexcept>
#include "parser/hpp/parser_runtime.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"

parser_runtime::parser_runtime() = default;

module_file parser_runtime::parse_module_file(const std::string& path) {
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

    return manifest_.visitor_.parse_module_file(ctx);
}

statement_file parser_runtime::parse_statement_file(const std::string& path) {
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

    return manifest_.visitor_.parse_statement_file(ctx);
}
