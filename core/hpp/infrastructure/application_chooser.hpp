#ifndef APPLICATION_CHOOSER_HPP
#define APPLICATION_CHOOSER_HPP

#include <cstddef>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/operand.hpp"
#include "value_objects/prim_id.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses an application with at least one feature operand: a primitive among the fillable ones, then each argument
// left to right among the pool's features of its type and then the constants of its type. While no feature has been
// chosen, the last argument whose type the pool holds features of offers its features alone. A fillable primitive has
// a candidate for every argument and a feature for one, so every choice has an option.
template<typename IGetNativePrimitive, typename IGetConstantsOfType, typename IChooseOption>
struct application_chooser {
    application_chooser(
        IGetNativePrimitive& get_native_primitive,
        IGetConstantsOfType& get_constants_of_type,
        IChooseOption& choose_option);
    derivation choose(
        const std::unordered_map<std::type_index, std::vector<feature_id>>& pool,
        const std::vector<prim_id>& fillable);
private:
    IGetNativePrimitive& get_native_primitive_;
    IGetConstantsOfType& get_constants_of_type_;
    IChooseOption& choose_option_;
    std::vector<feature_id> none_;
};

#define APPLICATION_CHOOSER_TEMPLATE \
    template<typename IGetNativePrimitive, typename IGetConstantsOfType, typename IChooseOption>
#define APPLICATION_CHOOSER application_chooser<IGetNativePrimitive, IGetConstantsOfType, IChooseOption>

APPLICATION_CHOOSER_TEMPLATE
APPLICATION_CHOOSER::application_chooser(
    IGetNativePrimitive& get_native_primitive,
    IGetConstantsOfType& get_constants_of_type,
    IChooseOption& choose_option)
    : get_native_primitive_(get_native_primitive)
    , get_constants_of_type_(get_constants_of_type)
    , choose_option_(choose_option)
    , none_() {}

APPLICATION_CHOOSER_TEMPLATE
derivation APPLICATION_CHOOSER::choose(
    const std::unordered_map<std::type_index, std::vector<feature_id>>& pool, const std::vector<prim_id>& fillable) {
    DEBUG_ASSERT(!fillable.empty());
    const prim_id prim = fillable[choose_option_.choose(fillable.size())];
    const std::vector<std::type_index>& arg_types = get_native_primitive_.primitive(prim).arg_types;
    std::vector<const std::vector<feature_id>*> features_by_arg;
    size_t last_feedable = 0;
    for (size_t position = 0; position < arg_types.size(); ++position) {
        const auto pooled = pool.find(arg_types[position]);
        features_by_arg.push_back(pooled == pool.end() ? &none_ : &pooled->second);
        if (features_by_arg.back()->empty())
            continue;
        last_feedable = position;
    }
    std::vector<operand> args;
    bool fed = false;
    for (size_t position = 0; position < arg_types.size(); ++position) {
        const std::vector<feature_id>& features = *features_by_arg[position];
        const std::vector<constant_id>& constants = get_constants_of_type_.of_type(arg_types[position]);
        const bool features_only = !fed && position == last_feedable;
        const size_t option_count = features.size() + (features_only ? 0 : constants.size());
        DEBUG_ASSERT(option_count > 0);
        const size_t option = choose_option_.choose(option_count);
        const bool from_pool = option < features.size();
        fed = fed || from_pool;
        args.push_back(from_pool
            ? operand{operand::from_feature{.id = features[option]}}
            : operand{operand::from_constant{.id = constants[option - features.size()]}});
    }
    return derivation{derivation::application{.prim = prim, .args = std::move(args)}};
}

#undef APPLICATION_CHOOSER
#undef APPLICATION_CHOOSER_TEMPLATE

}

#endif
