#ifndef OPERAND_TABLE_HPP
#define OPERAND_TABLE_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Numbers a synthesis session's operands: the constants first, then the session's features, each in its store's
// order. Every feature is heterogeneous and no constant is. Reads both stores in place and holds nothing itself.
template<typename IGetConstants, typename IGetConstantsOfType, typename IGetFeatures, typename IGetFeaturesOfType>
struct operand_table {
    operand_table(
        IGetConstants& get_constants,
        IGetConstantsOfType& get_constants_of_type,
        IGetFeatures& get_features,
        IGetFeaturesOfType& get_features_of_type);
    bool is_feature(size_t operand) const;
    size_t count_of_type(type_id type) const;
    size_t constant_count_of_type(type_id type) const;
    size_t operand_of_type(type_id type, size_t position) const;
    const feature* expression(size_t operand) const;
    const value* cell(size_t operand, size_t row) const;
private:
    IGetConstants& get_constants_;
    IGetConstantsOfType& get_constants_of_type_;
    IGetFeatures& get_features_;
    IGetFeaturesOfType& get_features_of_type_;
};

#define OPERAND_TABLE_TEMPLATE template<typename IGC, typename IGCT, typename IGF, typename IGFT>
#define OPERAND_TABLE          operand_table<IGC, IGCT, IGF, IGFT>

OPERAND_TABLE_TEMPLATE
OPERAND_TABLE::operand_table(
    IGC& get_constants, IGCT& get_constants_of_type, IGF& get_features, IGFT& get_features_of_type)
    : get_constants_(get_constants)
    , get_constants_of_type_(get_constants_of_type)
    , get_features_(get_features)
    , get_features_of_type_(get_features_of_type) {}

OPERAND_TABLE_TEMPLATE
bool OPERAND_TABLE::is_feature(size_t operand) const {
    return operand >= get_constants_.constants().size();
}

OPERAND_TABLE_TEMPLATE
size_t OPERAND_TABLE::count_of_type(type_id type) const {
    return get_constants_of_type_.constants_of_type(type).size() + get_features_of_type_.features_of_type(type).size();
}

OPERAND_TABLE_TEMPLATE
size_t OPERAND_TABLE::constant_count_of_type(type_id type) const {
    return get_constants_of_type_.constants_of_type(type).size();
}

OPERAND_TABLE_TEMPLATE
size_t OPERAND_TABLE::operand_of_type(type_id type, size_t position) const {
    const std::vector<size_t>& constants = get_constants_of_type_.constants_of_type(type);
    if (position < constants.size())
        return constants[position];
    const std::vector<size_t>& features = get_features_of_type_.features_of_type(type);
    DEBUG_ASSERT(position - constants.size() < features.size());
    return get_constants_.constants().size() + features[position - constants.size()];
}

OPERAND_TABLE_TEMPLATE
const feature* OPERAND_TABLE::expression(size_t operand) const {
    const std::vector<constant_operand>& constants = get_constants_.constants();
    if (operand < constants.size())
        return constants[operand].expression;
    return get_features_.candidates().at(operand - constants.size()).expression;
}

OPERAND_TABLE_TEMPLATE
const value* OPERAND_TABLE::cell(size_t operand, size_t row) const {
    const std::vector<constant_operand>& constants = get_constants_.constants();
    if (operand < constants.size())
        return constants[operand].literal;
    return get_features_.candidates().at(operand - constants.size()).observed.cells.at(row);
}

#undef OPERAND_TABLE
#undef OPERAND_TABLE_TEMPLATE

#endif
