#ifndef STATEMENT_ITERATOR_HPP
#define STATEMENT_ITERATOR_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

struct statement_iterator {
    statement_iterator(const std::vector<statement_file>& files);
    std::optional<statement> get_next_statement();

private:
    const std::vector<statement_file>& files_;
    size_t file_idx_;
    size_t item_idx_;
};

#endif
