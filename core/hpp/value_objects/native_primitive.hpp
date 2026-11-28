#ifndef NATIVE_PRIMITIVE_HPP
#define NATIVE_PRIMITIVE_HPP

#include <any>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <vector>
#include "infrastructure/lifted_primitive.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// A native function a column solver may apply, with the types of its arguments and result. `apply` takes one operand
// per argument, a column (a std::vector of its type in an any) or a scalar read at every row, and the row count, and
// gives the result column as a std::vector of the result type in an any. Not comparable, since it holds a function.
struct native_primitive {
    std::vector<std::type_index> arg_types;
    std::type_index result_type;
    std::function<std::any(const std::vector<operand_data>&, size_t)> apply;
    template<typename Ret, typename... Args>
    static native_primitive of(std::function<Ret(Args...)> function);
};

template<typename Ret, typename... Args>
native_primitive native_primitive::of(std::function<Ret(Args...)> function) {
    return native_primitive{
        .arg_types = {std::type_index(typeid(std::decay_t<Args>))...},
        .result_type = std::type_index(typeid(std::decay_t<Ret>)),
        .apply = lifted_primitive<Ret, Args...>(std::move(function))};
}

}

#endif
