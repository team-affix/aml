#ifndef SET_KEY_MEASURER_HPP
#define SET_KEY_MEASURER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"

namespace AML {

// Measures a set's key: the exclusive or of its members' keys, so a set has one key whatever the order of its members.
template<typename IGetFeatureRecord>
struct set_key_measurer {
    set_key_measurer(IGetFeatureRecord& get_feature_record);
    uint64_t measure(const std::vector<feature_id>& members) const;
private:
    IGetFeatureRecord& get_feature_record_;
};

#define SET_KEY_MEASURER_TEMPLATE template<typename IGetFeatureRecord>
#define SET_KEY_MEASURER          set_key_measurer<IGetFeatureRecord>

SET_KEY_MEASURER_TEMPLATE
SET_KEY_MEASURER::set_key_measurer(IGetFeatureRecord& get_feature_record)
    : get_feature_record_(get_feature_record) {}

SET_KEY_MEASURER_TEMPLATE
uint64_t SET_KEY_MEASURER::measure(const std::vector<feature_id>& members) const {
    uint64_t key = 0;
    for (const feature_id member : members)
        key ^= get_feature_record_.record(member).key;
    return key;
}

#undef SET_KEY_MEASURER
#undef SET_KEY_MEASURER_TEMPLATE

}

#endif
