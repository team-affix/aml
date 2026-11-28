#ifndef NATIVE_BIN_HPP
#define NATIVE_BIN_HPP

#include <cstddef>
#include <vector>
#include "value_objects/native_column.hpp"

namespace AML {

// Training data as native columns aligned by row: `inputs[v]` is variable v's column, `labels` each row's label, and
// row_count the number of rows. Not comparable, since its columns are not.
struct native_bin {
    std::vector<native_column> inputs;
    native_column labels;
    size_t row_count;
};

}

#endif
