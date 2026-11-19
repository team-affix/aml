#include "infrastructure/value_sizer.hpp"

size_t value_sizer::size(const value* term) const {
    size_t total = 1;
    for (const value* field : term->fields)
        total += size(field);
    return total;
}
