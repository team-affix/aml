#ifndef COLUMN_READER_HPP
#define COLUMN_READER_HPP

#include <any>
#include <memory>
#include <variant>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/cached_constant.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"
#include "value_objects/operand.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Reads a feature's cached column. A column missing from the cache is computed again from the feature's origin,
// reading its feature operands' columns the same way and its constant operands' cached values as scalars, and cached.
template<
    typename IGetFeatureRecord, typename IReadConstant, typename IFindCachedColumn, typename IComputeColumn,
    typename IMakeCachedColumn, typename IMeasureColumnBytes, typename IInsertCachedColumn>
struct column_reader {
    column_reader(
        IGetFeatureRecord& get_feature_record,
        IReadConstant& read_constant,
        IFindCachedColumn& find_cached_column,
        IComputeColumn& compute_column,
        IMakeCachedColumn& make_cached_column,
        IMeasureColumnBytes& measure_column_bytes,
        IInsertCachedColumn& insert_cached_column);
    std::shared_ptr<const cached_column> read(feature_id feature);
private:
    std::shared_ptr<const cached_column> cache_computed(
        feature_id feature, const feature_record& record, const std::vector<operand_data>& operands);
    IGetFeatureRecord& get_feature_record_;
    IReadConstant& read_constant_;
    IFindCachedColumn& find_cached_column_;
    IComputeColumn& compute_column_;
    IMakeCachedColumn& make_cached_column_;
    IMeasureColumnBytes& measure_column_bytes_;
    IInsertCachedColumn& insert_cached_column_;
};

#define COLUMN_READER_TEMPLATE \
    template< \
        typename IGetFeatureRecord, typename IReadConstant, typename IFindCachedColumn, typename IComputeColumn, \
        typename IMakeCachedColumn, typename IMeasureColumnBytes, typename IInsertCachedColumn>
#define COLUMN_READER \
    column_reader< \
        IGetFeatureRecord, IReadConstant, IFindCachedColumn, IComputeColumn, IMakeCachedColumn, \
        IMeasureColumnBytes, IInsertCachedColumn>

COLUMN_READER_TEMPLATE
COLUMN_READER::column_reader(
    IGetFeatureRecord& get_feature_record,
    IReadConstant& read_constant,
    IFindCachedColumn& find_cached_column,
    IComputeColumn& compute_column,
    IMakeCachedColumn& make_cached_column,
    IMeasureColumnBytes& measure_column_bytes,
    IInsertCachedColumn& insert_cached_column)
    : get_feature_record_(get_feature_record)
    , read_constant_(read_constant)
    , find_cached_column_(find_cached_column)
    , compute_column_(compute_column)
    , make_cached_column_(make_cached_column)
    , measure_column_bytes_(measure_column_bytes)
    , insert_cached_column_(insert_cached_column) {}

COLUMN_READER_TEMPLATE
std::shared_ptr<const cached_column> COLUMN_READER::read(feature_id feature) {
    std::shared_ptr<const cached_column> found = find_cached_column_.find(feature);
    if (found != nullptr)
        return found;
    const feature_record& record = get_feature_record_.record(feature);
    const derivation::application* application = std::get_if<derivation::application>(&record.origin.content);
    if (application == nullptr)
        return cache_computed(feature, record, {});
    std::vector<std::shared_ptr<const cached_column>> held;
    std::vector<std::shared_ptr<const cached_constant>> held_constants;
    std::vector<operand_data> operands;
    for (const operand& arg : application->args) {
        const operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
        if (argument == nullptr) {
            held_constants.push_back(read_constant_.read(std::get<operand::from_constant>(arg.source).id));
            operands.push_back(operand_data{.data = &held_constants.back()->cell, .scalar = true});
            continue;
        }
        held.push_back(read(argument->id));
        operands.push_back(operand_data{.data = &held.back()->cells, .scalar = false});
    }
    return cache_computed(feature, record, operands);
}

COLUMN_READER_TEMPLATE
std::shared_ptr<const cached_column> COLUMN_READER::cache_computed(
    feature_id feature, const feature_record& record, const std::vector<operand_data>& operands) {
    std::shared_ptr<const cached_column> made =
        make_cached_column_.make(record.type, compute_column_.compute(record.origin, operands));
    insert_cached_column_.insert(feature, made, measure_column_bytes_.measure(*made));
    return made;
}

#undef COLUMN_READER
#undef COLUMN_READER_TEMPLATE

}

#endif
