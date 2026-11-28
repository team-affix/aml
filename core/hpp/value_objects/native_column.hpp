#ifndef NATIVE_COLUMN_HPP
#define NATIVE_COLUMN_HPP

#include <any>
#include <typeindex>
#include <vector>

namespace AML {

// A column of native values: its type, and its cells as a std::vector of that type in an any. Not comparable, since
// an any is not.
struct native_column {
    std::type_index type;
    std::any cells;
    template<typename T>
    static native_column of(std::vector<T> cells);
};

template<typename T>
native_column native_column::of(std::vector<T> cells) {
    return native_column{.type = std::type_index(typeid(T)), .cells = std::any(std::move(cells))};
}

}

#endif
