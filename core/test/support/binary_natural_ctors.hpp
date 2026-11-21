#ifndef BINARY_NATURAL_CTORS_HPP
#define BINARY_NATURAL_CTORS_HPP

#include <compare>
#include "value_objects/ctor_id.hpp"

// The constructors of a user binary natural type: `zero`, `twice(n)` for 2n and `twice_plus_one(n)` for 2n + 1. A
// number has one form: zero is only `zero`, so `twice(zero)` never occurs.
struct binary_natural_ctors {
    AML::ctor_id zero;
    AML::ctor_id twice;
    AML::ctor_id twice_plus_one;
    auto operator<=>(const binary_natural_ctors&) const = default;
};

#endif
