#ifndef CONSTANT_SEEDER_HPP
#define CONSTANT_SEEDER_HPP

#include <vector>
#include "value_objects/constant_operand.hpp"
#include "value_objects/typed_constant.hpp"

// Seeds the constants the user offers, each as a constant feature.
template<typename IMakeConstantFeature, typename IInsertConstant>
struct constant_seeder {
    constant_seeder(IMakeConstantFeature& make_constant_feature, IInsertConstant& insert_constant);
    void seed(const std::vector<typed_constant>& constants);
private:
    IMakeConstantFeature& make_constant_feature_;
    IInsertConstant& insert_constant_;
};

#define CONSTANT_SEEDER_TEMPLATE template<typename IMakeConstantFeature, typename IInsertConstant>
#define CONSTANT_SEEDER          constant_seeder<IMakeConstantFeature, IInsertConstant>

CONSTANT_SEEDER_TEMPLATE
CONSTANT_SEEDER::constant_seeder(IMakeConstantFeature& make_constant_feature, IInsertConstant& insert_constant)
    : make_constant_feature_(make_constant_feature)
    , insert_constant_(insert_constant) {}

CONSTANT_SEEDER_TEMPLATE
void CONSTANT_SEEDER::seed(const std::vector<typed_constant>& constants) {
    for (const typed_constant& constant : constants) {
        insert_constant_.insert(constant_operand{
            .expression = make_constant_feature_.make_constant(constant.literal),
            .literal = constant.literal,
            .type = constant.type});
    }
}

#undef CONSTANT_SEEDER
#undef CONSTANT_SEEDER_TEMPLATE

#endif
