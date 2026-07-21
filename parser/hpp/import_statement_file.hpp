#ifndef IMPORT_STATEMENT_FILE_HPP
#define IMPORT_STATEMENT_FILE_HPP

#include <fstream>
#include <stdexcept>
#include <string>
#include "../generated/AMLLexer.h"
#include "../generated/AMLParser.h"
#include "../hpp/aml_visitor.hpp"
#include "value_objects/statement_file.hpp"

template<typename IMakeAmlAbs,
         typename IMakeAmlApp,
         typename IMakeAmlSymbol,
         typename IMakeAmlNat,
         typename IMakeAmlInteger,
         typename IMakeAmlCharacter,
         typename IMakeAmlString,
         typename IMakeAmlList>
statement_file import_statement_file(const std::string& path,
                                     IMakeAmlAbs& make_aml_abs,
                                     IMakeAmlApp& make_aml_app,
                                     IMakeAmlSymbol& make_aml_symbol,
                                     IMakeAmlNat& make_aml_nat,
                                     IMakeAmlInteger& make_aml_integer,
                                     IMakeAmlCharacter& make_aml_character,
                                     IMakeAmlString& make_aml_string,
                                     IMakeAmlList& make_aml_list) {
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

    aml_visitor<IMakeAmlAbs, IMakeAmlApp, IMakeAmlSymbol, IMakeAmlNat,
                IMakeAmlInteger, IMakeAmlCharacter, IMakeAmlString, IMakeAmlList>
        visitor{make_aml_abs, make_aml_app, make_aml_symbol, make_aml_nat,
                make_aml_integer, make_aml_character, make_aml_string, make_aml_list};
    return visitor.parse_statement_file(ctx);
}

#endif
