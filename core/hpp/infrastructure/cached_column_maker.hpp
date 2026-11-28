#ifndef CACHED_COLUMN_MAKER_HPP
#define CACHED_COLUMN_MAKER_HPP

#include <any>
#include <memory>
#include <typeindex>
#include "value_objects/cached_column.hpp"
#include "value_objects/native_type.hpp"

namespace AML {

// Makes a cached column of a feature's values, a std::vector of its type in an any: classes each row and hashes the
// column with its native type's operations.
template<typename IGetNativeType>
struct cached_column_maker {
    cached_column_maker(IGetNativeType& get_native_type);
    std::shared_ptr<const cached_column> make(std::type_index type, std::any cells) const;
private:
    IGetNativeType& get_native_type_;
};

#define CACHED_COLUMN_MAKER_TEMPLATE template<typename IGetNativeType>
#define CACHED_COLUMN_MAKER          cached_column_maker<IGetNativeType>

CACHED_COLUMN_MAKER_TEMPLATE
CACHED_COLUMN_MAKER::cached_column_maker(IGetNativeType& get_native_type)
    : get_native_type_(get_native_type) {}

CACHED_COLUMN_MAKER_TEMPLATE
std::shared_ptr<const cached_column> CACHED_COLUMN_MAKER::make(std::type_index type, std::any cells) const {
    const native_type& operations = get_native_type_.type(type);
    std::vector<size_t> classes = operations.classify(cells);
    const size_t hash = operations.hash(cells);
    return std::make_shared<const cached_column>(
        cached_column{.type = type, .cells = std::move(cells), .classes = std::move(classes), .hash = hash});
}

#undef CACHED_COLUMN_MAKER
#undef CACHED_COLUMN_MAKER_TEMPLATE

}

#endif
