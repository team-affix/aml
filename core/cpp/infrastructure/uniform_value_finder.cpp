#include "infrastructure/uniform_value_finder.hpp"

namespace AML {

std::optional<const value*> uniform_value_finder::find(const column& cells) const {
    if (cells.empty())
        return std::nullopt;
    const value* first = cells.front();
    for (const value* cell : cells) {
        if (cell != first)
            return std::nullopt;
    }
    return first;
}

}
