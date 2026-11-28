#ifndef COLUMN_BYTE_MEASURER_HPP
#define COLUMN_BYTE_MEASURER_HPP

#include <cstddef>
#include "value_objects/cached_column.hpp"
#include "value_objects/native_type.hpp"

namespace AML {

// Measures the bytes a cached column holds in place: its type's cell bytes and one class per row. Memory a cell owns
// elsewhere, such as a vector's elements, is not counted.
template<typename IGetNativeType>
struct column_byte_measurer {
    column_byte_measurer(IGetNativeType& get_native_type);
    size_t measure(const cached_column& cached) const;
private:
    IGetNativeType& get_native_type_;
};

#define COLUMN_BYTE_MEASURER_TEMPLATE template<typename IGetNativeType>
#define COLUMN_BYTE_MEASURER          column_byte_measurer<IGetNativeType>

COLUMN_BYTE_MEASURER_TEMPLATE
COLUMN_BYTE_MEASURER::column_byte_measurer(IGetNativeType& get_native_type)
    : get_native_type_(get_native_type) {}

COLUMN_BYTE_MEASURER_TEMPLATE
size_t COLUMN_BYTE_MEASURER::measure(const cached_column& cached) const {
    const size_t row_count = cached.classes.size();
    return row_count * (get_native_type_.type(cached.type).cell_bytes + sizeof(size_t));
}

#undef COLUMN_BYTE_MEASURER
#undef COLUMN_BYTE_MEASURER_TEMPLATE

}

#endif
