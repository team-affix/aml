#ifndef UNIFORM_VALUE_FINDER_HPP
#define UNIFORM_VALUE_FINDER_HPP

#include <optional>
#include "value_objects/column.hpp"
#include "value_objects/value.hpp"

namespace AML {

// Finds the one value every cell of a column holds. A column with no cells, or with two different values, has none.
struct uniform_value_finder {
    std::optional<const value*> find(const column& cells) const;
};

}

#endif
