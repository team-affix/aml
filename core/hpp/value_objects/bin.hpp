#ifndef BIN_HPP
#define BIN_HPP

#include <vector>
#include "value_objects/typed_column.hpp"

namespace AML {

// A set of data points seen as columns. `inputs[v]` holds variable v's value in every row: the data points' own
// inputs first, then every argument bound by the matches above the bin. `labels` holds each row's label, typed like
// the inputs so a feature can be compared with it.
struct bin {
    std::vector<typed_column> inputs;
    typed_column labels;
    auto operator<=>(const bin&) const = default;
};

}

#endif
