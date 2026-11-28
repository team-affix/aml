#ifndef EQUAL_FEATURE_FINDER_HPP
#define EQUAL_FEATURE_FINDER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include "value_objects/cached_column.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_type.hpp"

namespace AML {

// Finds a feature whose column equals a given one in type and in every row's value. Features with the column's hash
// are checked in the order they were indexed: equal columns have equal classes, and then every row is compared.
template<typename IFindHashedFeatures, typename IReadColumn, typename IGetNativeType>
struct equal_feature_finder {
    equal_feature_finder(
        IFindHashedFeatures& find_hashed_features, IReadColumn& read_column, IGetNativeType& get_native_type);
    std::optional<feature_id> find(const cached_column& made);
private:
    IFindHashedFeatures& find_hashed_features_;
    IReadColumn& read_column_;
    IGetNativeType& get_native_type_;
};

#define EQUAL_FEATURE_FINDER_TEMPLATE \
    template<typename IFindHashedFeatures, typename IReadColumn, typename IGetNativeType>
#define EQUAL_FEATURE_FINDER equal_feature_finder<IFindHashedFeatures, IReadColumn, IGetNativeType>

EQUAL_FEATURE_FINDER_TEMPLATE
EQUAL_FEATURE_FINDER::equal_feature_finder(
    IFindHashedFeatures& find_hashed_features, IReadColumn& read_column, IGetNativeType& get_native_type)
    : find_hashed_features_(find_hashed_features)
    , read_column_(read_column)
    , get_native_type_(get_native_type) {}

EQUAL_FEATURE_FINDER_TEMPLATE
std::optional<feature_id> EQUAL_FEATURE_FINDER::find(const cached_column& made) {
    for (const feature_id feature : find_hashed_features_.find(made.hash)) {
        const std::shared_ptr<const cached_column> held = read_column_.read(feature);
        if (held->type != made.type || held->classes != made.classes)
            continue;
        const native_type& operations = get_native_type_.type(made.type);
        bool equal = true;
        for (size_t row = 0; row < made.classes.size() && equal; ++row)
            equal = operations.equal_at(held->cells, row, made.cells, row);
        if (equal)
            return feature;
    }
    return std::nullopt;
}

#undef EQUAL_FEATURE_FINDER
#undef EQUAL_FEATURE_FINDER_TEMPLATE

}

#endif
