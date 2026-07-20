#ifndef STATEMENT_FILE_HPP
#define STATEMENT_FILE_HPP

#include <vector>
#include "value_objects/statement.hpp"

struct statement_file {
    std::vector<statement> statements;
    auto operator<=>(const statement_file&) const = default;
};

#endif
