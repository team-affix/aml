#ifndef COLUMN_VIEW_HPP
#define COLUMN_VIEW_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include "value_objects/column.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/typed_column.hpp"

namespace AML {

// A typed column read through a selection of rows: its row r is `(*cells)[(*rows)[r]]`. Views share their cells and
// rows, so a bin split from another reads its parent's cells instead of copying them. Two views are equal when their
// types and the values they read are equal, whatever cells and rows they share; that is why equality is not defaulted.
struct column_view {
    type_id type;
    std::shared_ptr<const column> cells;
    std::shared_ptr<const std::vector<size_t>> rows;
    bool operator==(const column_view& other) const;
    // A view of every cell of `dense`, in order.
    static column_view of(typed_column dense);
};

}

#endif
