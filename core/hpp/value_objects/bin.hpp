#ifndef BIN_HPP
#define BIN_HPP

#include <vector>
#include "value_objects/column.hpp"
#include "value_objects/typed_column.hpp"

// A set of data points seen as columns. `inputs[v]` holds variable v's value in every row: the data points' own
// inputs first, then every argument bound by the matches above the bin. `labels` holds each row's label.
struct bin {
    std::vector<typed_column> inputs;
    column labels;
    auto operator<=>(const bin&) const = default;
};

#endif
