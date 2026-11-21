#include "infrastructure/uniform_value_finder.hpp"

namespace AML {

std::optional<const value*> uniform_value_finder::find(const column_view& observed) const {
    if (observed.rows->empty())
        return std::nullopt;
    const value& first = (*observed.cells)[observed.rows->front()];
    for (const size_t row : *observed.rows) {
        if ((*observed.cells)[row] != first)
            return std::nullopt;
    }
    return &first;
}

}
