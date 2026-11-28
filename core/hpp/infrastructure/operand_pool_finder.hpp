#ifndef OPERAND_POOL_FINDER_HPP
#define OPERAND_POOL_FINDER_HPP

#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"

namespace AML {

// Finds the feature operands an ancestry offers, by type: each type lists the ancestry's features of that type in
// order.
template<typename IGetFeatureRecord>
struct operand_pool_finder {
    operand_pool_finder(IGetFeatureRecord& get_feature_record);
    std::unordered_map<std::type_index, std::vector<feature_id>> find(const std::vector<feature_id>& ancestry) const;
private:
    IGetFeatureRecord& get_feature_record_;
};

#define OPERAND_POOL_FINDER_TEMPLATE template<typename IGetFeatureRecord>
#define OPERAND_POOL_FINDER          operand_pool_finder<IGetFeatureRecord>

OPERAND_POOL_FINDER_TEMPLATE
OPERAND_POOL_FINDER::operand_pool_finder(IGetFeatureRecord& get_feature_record)
    : get_feature_record_(get_feature_record) {}

OPERAND_POOL_FINDER_TEMPLATE
std::unordered_map<std::type_index, std::vector<feature_id>> OPERAND_POOL_FINDER::find(
    const std::vector<feature_id>& ancestry) const {
    std::unordered_map<std::type_index, std::vector<feature_id>> pool;
    for (const feature_id feature : ancestry)
        pool[get_feature_record_.record(feature).type].push_back(feature);
    return pool;
}

#undef OPERAND_POOL_FINDER
#undef OPERAND_POOL_FINDER_TEMPLATE

}

#endif
