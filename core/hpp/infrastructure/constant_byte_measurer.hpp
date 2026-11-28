#ifndef CONSTANT_BYTE_MEASURER_HPP
#define CONSTANT_BYTE_MEASURER_HPP

#include <cstddef>
#include "value_objects/cached_constant.hpp"
#include "value_objects/native_type.hpp"

namespace AML {

// Measures the bytes a cached constant holds in place: its type's cell bytes. Memory the value owns elsewhere, such as
// a vector's elements, is not counted.
template<typename IGetNativeType>
struct constant_byte_measurer {
    constant_byte_measurer(IGetNativeType& get_native_type);
    size_t measure(const cached_constant& cached) const;
private:
    IGetNativeType& get_native_type_;
};

#define CONSTANT_BYTE_MEASURER_TEMPLATE template<typename IGetNativeType>
#define CONSTANT_BYTE_MEASURER          constant_byte_measurer<IGetNativeType>

CONSTANT_BYTE_MEASURER_TEMPLATE
CONSTANT_BYTE_MEASURER::constant_byte_measurer(IGetNativeType& get_native_type)
    : get_native_type_(get_native_type) {}

CONSTANT_BYTE_MEASURER_TEMPLATE
size_t CONSTANT_BYTE_MEASURER::measure(const cached_constant& cached) const {
    return get_native_type_.type(cached.type).cell_bytes;
}

#undef CONSTANT_BYTE_MEASURER
#undef CONSTANT_BYTE_MEASURER_TEMPLATE

}

#endif
