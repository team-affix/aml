#ifndef CONSTANT_READER_HPP
#define CONSTANT_READER_HPP

#include <memory>
#include <variant>
#include <vector>
#include "value_objects/cached_constant.hpp"
#include "value_objects/constant_derivation.hpp"
#include "value_objects/constant_id.hpp"
#include "value_objects/constant_record.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Reads a constant's cached value. A value missing from the cache is computed again from the constant's origin,
// reading its arguments' values the same way, and cached.
template<
    typename IGetConstantRecord, typename IFindCachedConstant, typename IComputeConstant,
    typename IMeasureConstantBytes, typename IInsertCachedConstant>
struct constant_reader {
    constant_reader(
        IGetConstantRecord& get_constant_record,
        IFindCachedConstant& find_cached_constant,
        IComputeConstant& compute_constant,
        IMeasureConstantBytes& measure_constant_bytes,
        IInsertCachedConstant& insert_cached_constant);
    std::shared_ptr<const cached_constant> read(constant_id constant);
private:
    std::shared_ptr<const cached_constant> cache_computed(
        constant_id constant, const constant_record& record, const std::vector<operand_data>& operands);
    IGetConstantRecord& get_constant_record_;
    IFindCachedConstant& find_cached_constant_;
    IComputeConstant& compute_constant_;
    IMeasureConstantBytes& measure_constant_bytes_;
    IInsertCachedConstant& insert_cached_constant_;
};

#define CONSTANT_READER_TEMPLATE \
    template< \
        typename IGetConstantRecord, typename IFindCachedConstant, typename IComputeConstant, \
        typename IMeasureConstantBytes, typename IInsertCachedConstant>
#define CONSTANT_READER \
    constant_reader< \
        IGetConstantRecord, IFindCachedConstant, IComputeConstant, IMeasureConstantBytes, IInsertCachedConstant>

CONSTANT_READER_TEMPLATE
CONSTANT_READER::constant_reader(
    IGetConstantRecord& get_constant_record,
    IFindCachedConstant& find_cached_constant,
    IComputeConstant& compute_constant,
    IMeasureConstantBytes& measure_constant_bytes,
    IInsertCachedConstant& insert_cached_constant)
    : get_constant_record_(get_constant_record)
    , find_cached_constant_(find_cached_constant)
    , compute_constant_(compute_constant)
    , measure_constant_bytes_(measure_constant_bytes)
    , insert_cached_constant_(insert_cached_constant) {}

CONSTANT_READER_TEMPLATE
std::shared_ptr<const cached_constant> CONSTANT_READER::read(constant_id constant) {
    std::shared_ptr<const cached_constant> found = find_cached_constant_.find(constant);
    if (found != nullptr)
        return found;
    const constant_record& record = get_constant_record_.record(constant);
    const constant_derivation::application* application =
        std::get_if<constant_derivation::application>(&record.origin.content);
    if (application == nullptr)
        return cache_computed(constant, record, {});
    std::vector<std::shared_ptr<const cached_constant>> held;
    std::vector<operand_data> operands;
    for (const constant_id argument : application->args) {
        held.push_back(read(argument));
        operands.push_back(operand_data{.data = &held.back()->cell, .scalar = true});
    }
    return cache_computed(constant, record, operands);
}

CONSTANT_READER_TEMPLATE
std::shared_ptr<const cached_constant> CONSTANT_READER::cache_computed(
    constant_id constant, const constant_record& record, const std::vector<operand_data>& operands) {
    std::shared_ptr<const cached_constant> made = std::make_shared<const cached_constant>(
        cached_constant{.type = record.type, .cell = compute_constant_.compute(record.origin, operands)});
    insert_cached_constant_.insert(constant, made, measure_constant_bytes_.measure(*made));
    return made;
}

#undef CONSTANT_READER
#undef CONSTANT_READER_TEMPLATE

}

#endif
