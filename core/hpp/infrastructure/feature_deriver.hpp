#ifndef FEATURE_DERIVER_HPP
#define FEATURE_DERIVER_HPP

#include <any>
#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/cached_constant.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/operand.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Derives the feature an application makes. A derivation made before gives its feature again. Otherwise its column is
// computed from its feature operands' columns and its constant operands' cached values as scalars: a column holding
// one value everywhere gives no feature; a column equal to a feature's gives that feature, whose origin becomes the
// application if that is cheaper; any other column becomes a new feature.
template<
    typename IFindDerivation, typename IReadColumn, typename IReadConstant, typename IComputeColumn,
    typename IGetNativePrimitive, typename IMakeCachedColumn, typename IDetectUniformColumn, typename IFindEqualFeature,
    typename ITryReplaceOrigin, typename IInsertDerivation, typename IRecordFeature>
struct feature_deriver {
    feature_deriver(
        IFindDerivation& find_derivation,
        IReadColumn& read_column,
        IReadConstant& read_constant,
        IComputeColumn& compute_column,
        IGetNativePrimitive& get_native_primitive,
        IMakeCachedColumn& make_cached_column,
        IDetectUniformColumn& detect_uniform_column,
        IFindEqualFeature& find_equal_feature,
        ITryReplaceOrigin& try_replace_origin,
        IInsertDerivation& insert_derivation,
        IRecordFeature& record_feature);
    std::optional<feature_id> derive(const derivation& origin);
private:
    IFindDerivation& find_derivation_;
    IReadColumn& read_column_;
    IReadConstant& read_constant_;
    IComputeColumn& compute_column_;
    IGetNativePrimitive& get_native_primitive_;
    IMakeCachedColumn& make_cached_column_;
    IDetectUniformColumn& detect_uniform_column_;
    IFindEqualFeature& find_equal_feature_;
    ITryReplaceOrigin& try_replace_origin_;
    IInsertDerivation& insert_derivation_;
    IRecordFeature& record_feature_;
};

#define FEATURE_DERIVER_TEMPLATE \
    template< \
        typename IFindDerivation, typename IReadColumn, typename IReadConstant, typename IComputeColumn, \
        typename IGetNativePrimitive, typename IMakeCachedColumn, typename IDetectUniformColumn, \
        typename IFindEqualFeature, typename ITryReplaceOrigin, typename IInsertDerivation, typename IRecordFeature>
#define FEATURE_DERIVER \
    feature_deriver< \
        IFindDerivation, IReadColumn, IReadConstant, IComputeColumn, IGetNativePrimitive, IMakeCachedColumn, \
        IDetectUniformColumn, IFindEqualFeature, ITryReplaceOrigin, IInsertDerivation, IRecordFeature>

FEATURE_DERIVER_TEMPLATE
FEATURE_DERIVER::feature_deriver(
    IFindDerivation& find_derivation,
    IReadColumn& read_column,
    IReadConstant& read_constant,
    IComputeColumn& compute_column,
    IGetNativePrimitive& get_native_primitive,
    IMakeCachedColumn& make_cached_column,
    IDetectUniformColumn& detect_uniform_column,
    IFindEqualFeature& find_equal_feature,
    ITryReplaceOrigin& try_replace_origin,
    IInsertDerivation& insert_derivation,
    IRecordFeature& record_feature)
    : find_derivation_(find_derivation)
    , read_column_(read_column)
    , read_constant_(read_constant)
    , compute_column_(compute_column)
    , get_native_primitive_(get_native_primitive)
    , make_cached_column_(make_cached_column)
    , detect_uniform_column_(detect_uniform_column)
    , find_equal_feature_(find_equal_feature)
    , try_replace_origin_(try_replace_origin)
    , insert_derivation_(insert_derivation)
    , record_feature_(record_feature) {}

FEATURE_DERIVER_TEMPLATE
std::optional<feature_id> FEATURE_DERIVER::derive(const derivation& origin) {
    const std::optional<feature_id> known = find_derivation_.find(origin);
    if (known.has_value())
        return known;
    const derivation::application& application = std::get<derivation::application>(origin.content);
    std::vector<std::shared_ptr<const cached_column>> held;
    std::vector<std::shared_ptr<const cached_constant>> held_constants;
    std::vector<operand_data> operands;
    for (const operand& arg : application.args) {
        const operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
        if (argument == nullptr) {
            held_constants.push_back(read_constant_.read(std::get<operand::from_constant>(arg.source).id));
            operands.push_back(operand_data{.data = &held_constants.back()->cell, .scalar = true});
            continue;
        }
        held.push_back(read_column_.read(argument->id));
        operands.push_back(operand_data{.data = &held.back()->cells, .scalar = false});
    }
    std::shared_ptr<const cached_column> made = make_cached_column_.make(
        get_native_primitive_.primitive(application.prim).result_type, compute_column_.compute(origin, operands));
    if (detect_uniform_column_.detect(*made))
        return std::nullopt;
    const std::optional<feature_id> equal = find_equal_feature_.find(*made);
    if (!equal.has_value())
        return record_feature_.record(origin, std::move(made));
    try_replace_origin_.try_replace(equal.value(), origin);
    insert_derivation_.insert(origin, equal.value());
    return equal;
}

#undef FEATURE_DERIVER
#undef FEATURE_DERIVER_TEMPLATE

}

#endif
