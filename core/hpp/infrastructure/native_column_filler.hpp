#ifndef NATIVE_COLUMN_FILLER_HPP
#define NATIVE_COLUMN_FILLER_HPP

#include <any>
#include <cstddef>
#include <vector>

namespace AML {

// Fills a column of T with one value, given in an any, in every row; the column is a std::vector<T> in an any.
template<typename T>
struct native_column_filler {
    std::any operator()(const std::any& literal, size_t row_count) const;
};

#define NATIVE_COLUMN_FILLER_TEMPLATE template<typename T>
#define NATIVE_COLUMN_FILLER          native_column_filler<T>

NATIVE_COLUMN_FILLER_TEMPLATE
std::any NATIVE_COLUMN_FILLER::operator()(const std::any& literal, size_t row_count) const {
    return std::any(std::vector<T>(row_count, std::any_cast<const T&>(literal)));
}

#undef NATIVE_COLUMN_FILLER
#undef NATIVE_COLUMN_FILLER_TEMPLATE

}

#endif
