#ifndef VALUE_HPP
#define VALUE_HPP

#include <cstddef>
#include <vector>

namespace AML {

// A constructor applied to argument values. `tag` is the constructor's position among its type's constructors; the
// type itself is known from where the value sits (its column), not stored. Values are interned, so two values are equal
// exactly when their pointers are.
struct value {
    size_t tag;
    std::vector<const value*> args;
    auto operator<=>(const value&) const = default;
};

}

#endif
