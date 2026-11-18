#ifndef PRIMITIVE_DISPATCHER_HPP
#define PRIMITIVE_DISPATCHER_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/value.hpp"

// Dispatches calls to the primitive behind a primitive id. A primitive's id is its position in the list.
template<typename IPrimitive>
struct primitive_dispatcher {
    primitive_dispatcher(const std::vector<IPrimitive>& primitives);
    size_t count() const;
    primitive_signature signature(prim_id prim) const;
    const value* apply(prim_id prim, const std::vector<const value*>& args) const;
private:
    std::vector<IPrimitive> primitives_;
};

#define PRIMITIVE_DISPATCHER_TEMPLATE template<typename IPrimitive>
#define PRIMITIVE_DISPATCHER          primitive_dispatcher<IPrimitive>

PRIMITIVE_DISPATCHER_TEMPLATE
PRIMITIVE_DISPATCHER::primitive_dispatcher(const std::vector<IPrimitive>& primitives)
    : primitives_(primitives) {}

PRIMITIVE_DISPATCHER_TEMPLATE
size_t PRIMITIVE_DISPATCHER::count() const {
    return primitives_.size();
}

PRIMITIVE_DISPATCHER_TEMPLATE
primitive_signature PRIMITIVE_DISPATCHER::signature(prim_id prim) const {
    return std::visit([](const auto& primitive) { return primitive->signature(); }, primitives_.at(prim));
}

PRIMITIVE_DISPATCHER_TEMPLATE
const value* PRIMITIVE_DISPATCHER::apply(prim_id prim, const std::vector<const value*>& args) const {
    return std::visit([&args](const auto& primitive) { return primitive->apply(args); }, primitives_.at(prim));
}

#undef PRIMITIVE_DISPATCHER
#undef PRIMITIVE_DISPATCHER_TEMPLATE

#endif
