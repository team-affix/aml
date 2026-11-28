#ifndef ROW_KEY_NUMBERER_HPP
#define ROW_KEY_NUMBERER_HPP

#include <cstddef>
#include <vector>

namespace AML {

// Numbers the rows' keys over every prefix of a list of class columns. Layer i gives each row the number of its key
// over the first i columns: rows with equal classes in all of them share a number, and numbers run from 0 in order of
// first appearance, so each is below the row count. Layer 0, the empty prefix, numbers every row 0. Layer i is numbered
// from layer i - 1 and column i - 1 alone, so all layers cost one pass per column.
struct row_key_numberer {
    std::vector<std::vector<size_t>> number(
        const std::vector<const std::vector<size_t>*>& columns, size_t row_count) const;
};

}

#endif
