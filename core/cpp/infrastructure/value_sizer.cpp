#include "infrastructure/value_sizer.hpp"

namespace AML {

size_t value_sizer::size(const value& term) const {
    size_t total = 1;
    for (const value& argument : term.args)
        total += size(argument);
    return total;
}

}
