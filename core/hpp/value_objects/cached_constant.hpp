#ifndef CACHED_CONSTANT_HPP
#define CACHED_CONSTANT_HPP

#include <any>
#include <typeindex>

namespace AML {

// A constant's one value: its type and the value itself, one cell of that type in an any. Not comparable, since an
// any is not.
struct cached_constant {
    std::type_index type;
    std::any cell;
};

}

#endif
