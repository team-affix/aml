#ifndef NATIVE_CONSTANT_HPP
#define NATIVE_CONSTANT_HPP

#include <any>
#include <typeindex>

namespace AML {

// A native value a column solver may use as a primitive argument, with its type. Not comparable, since an any is not.
struct native_constant {
    std::type_index type;
    std::any literal;
    template<typename T>
    static native_constant of(T literal);
};

template<typename T>
native_constant native_constant::of(T literal) {
    return native_constant{.type = std::type_index(typeid(T)), .literal = std::any(std::move(literal))};
}

}

#endif
