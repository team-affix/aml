#ifndef PRIMITIVE_SIGNATURE_READER_HPP
#define PRIMITIVE_SIGNATURE_READER_HPP

#include <variant>
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"

// Reads the signature of the primitive behind an id.
template<typename IGetPrimitive>
struct primitive_signature_reader {
    primitive_signature_reader(IGetPrimitive& get_primitive);
    primitive_signature signature(prim_id prim) const;
private:
    IGetPrimitive& get_primitive_;
};

#define PRIMITIVE_SIGNATURE_READER_TEMPLATE template<typename IGetPrimitive>
#define PRIMITIVE_SIGNATURE_READER          primitive_signature_reader<IGetPrimitive>

PRIMITIVE_SIGNATURE_READER_TEMPLATE
PRIMITIVE_SIGNATURE_READER::primitive_signature_reader(IGetPrimitive& get_primitive)
    : get_primitive_(get_primitive) {}

PRIMITIVE_SIGNATURE_READER_TEMPLATE
primitive_signature PRIMITIVE_SIGNATURE_READER::signature(prim_id prim) const {
    return std::visit([](const auto& primitive) { return primitive->signature(); }, get_primitive_.primitive_at(prim));
}

#undef PRIMITIVE_SIGNATURE_READER
#undef PRIMITIVE_SIGNATURE_READER_TEMPLATE

#endif
