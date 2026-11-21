#ifndef BIN_VIEW_HPP
#define BIN_VIEW_HPP

#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/column_view.hpp"

namespace AML {

// A bin as model building sees it: `inputs[v]` is variable v's column (the data points' own inputs first, then every
// argument bound by the matches above the bin) and `labels` each row's label, all aligned by row. The columns are
// views, so a bin split from another shares its parent's cells and only selects rows.
struct bin_view {
    std::vector<column_view> inputs;
    column_view labels;
    bool operator==(const bin_view&) const = default;
    // A view of every row of `dense`; its columns share one selection of rows.
    static bin_view of(const bin& dense);
};

}

#endif
