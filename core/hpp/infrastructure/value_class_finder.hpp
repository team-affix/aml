#ifndef VALUE_CLASS_FINDER_HPP
#define VALUE_CLASS_FINDER_HPP

#include <any>
#include <cstddef>
#include <optional>
#include "value_objects/cached_column.hpp"
#include "value_objects/native_type.hpp"

namespace AML {

// Finds the class a value takes in a cached column: the class of the first row holding it. The value is given as a
// one-row column of the column's type. A value no row holds has no class.
template<typename IGetNativeType>
struct value_class_finder {
    value_class_finder(IGetNativeType& get_native_type);
    std::optional<size_t> find(const cached_column& column, const std::any& one_row) const;
private:
    IGetNativeType& get_native_type_;
};

#define VALUE_CLASS_FINDER_TEMPLATE template<typename IGetNativeType>
#define VALUE_CLASS_FINDER          value_class_finder<IGetNativeType>

VALUE_CLASS_FINDER_TEMPLATE
VALUE_CLASS_FINDER::value_class_finder(IGetNativeType& get_native_type)
    : get_native_type_(get_native_type) {}

VALUE_CLASS_FINDER_TEMPLATE
std::optional<size_t> VALUE_CLASS_FINDER::find(const cached_column& column, const std::any& one_row) const {
    const native_type& operations = get_native_type_.type(column.type);
    for (size_t row = 0; row < column.classes.size(); ++row) {
        if (operations.equal_at(column.cells, row, one_row, 0))
            return column.classes[row];
    }
    return std::nullopt;
}

#undef VALUE_CLASS_FINDER
#undef VALUE_CLASS_FINDER_TEMPLATE

}

#endif
