#include "infrastructure/uniform_value_finder.hpp"

namespace AML {

std::optional<const value*> uniform_value_finder::find(const column_view& cells) const {
    if (cells.rows->empty())
        return std::nullopt;
    const value& first = (*cells.cells)[cells.rows->front()];
    for (const size_t row : *cells.rows) {
        if ((*cells.cells)[row] != first)
            return std::nullopt;
    }
    return &first;
}

}
