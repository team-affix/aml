#ifndef CONSTANT_SEEDER_HPP
#define CONSTANT_SEEDER_HPP

#include <cstddef>
#include "value_objects/constant_operand.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"

// Seeds the constants: every value of every type up to a size, each as a constant feature.
template<typename IGetTypeCount, typename IEnumerateValues, typename IMakeConstantFeature, typename IInsertConstant>
struct constant_seeder {
    constant_seeder(
        IGetTypeCount& get_type_count,
        IEnumerateValues& enumerate_values,
        IMakeConstantFeature& make_constant_feature,
        IInsertConstant& insert_constant);
    void seed(size_t max_size);
private:
    IGetTypeCount& get_type_count_;
    IEnumerateValues& enumerate_values_;
    IMakeConstantFeature& make_constant_feature_;
    IInsertConstant& insert_constant_;
};

#define CONSTANT_SEEDER_TEMPLATE \
    template<typename IGetTypeCount, typename IEnumerateValues, typename IMakeConstantFeature, typename IInsertConstant>
#define CONSTANT_SEEDER constant_seeder<IGetTypeCount, IEnumerateValues, IMakeConstantFeature, IInsertConstant>

CONSTANT_SEEDER_TEMPLATE
CONSTANT_SEEDER::constant_seeder(
    IGetTypeCount& get_type_count,
    IEnumerateValues& enumerate_values,
    IMakeConstantFeature& make_constant_feature,
    IInsertConstant& insert_constant)
    : get_type_count_(get_type_count)
    , enumerate_values_(enumerate_values)
    , make_constant_feature_(make_constant_feature)
    , insert_constant_(insert_constant) {}

CONSTANT_SEEDER_TEMPLATE
void CONSTANT_SEEDER::seed(size_t max_size) {
    for (type_id type = 0; type < get_type_count_.type_count(); ++type) {
        for (const value* literal : enumerate_values_.enumerate(type, max_size)) {
            insert_constant_.insert(constant_operand{
                .expression = make_constant_feature_.make_constant(literal),
                .literal = literal,
                .type = type});
        }
    }
}

#undef CONSTANT_SEEDER
#undef CONSTANT_SEEDER_TEMPLATE

#endif
