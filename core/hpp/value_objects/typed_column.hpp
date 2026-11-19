#ifndef TYPED_COLUMN_HPP
#define TYPED_COLUMN_HPP

#include "value_objects/column.hpp"
#include "value_objects/type_id.hpp"

struct typed_column {
    type_id type;
    column cells;
    auto operator<=>(const typed_column&) const = default;
};

#endif
