#ifndef VALUE_SIZER_HPP
#define VALUE_SIZER_HPP

#include <cstddef>
#include "value_objects/value.hpp"

namespace AML {

// Sizes a value: one per constructor in it.
struct value_sizer {
    size_t size(const value& term) const;
};

}

#endif
