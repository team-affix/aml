#ifndef DERIVATION_SAMPLER_HPP
#define DERIVATION_SAMPLER_HPP

#include <cstddef>
#include <optional>
#include <random>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/operand.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Samples an application with at least one feature operand: a primitive uniformly among the eligible ones, then each
// argument uniformly among the pool's features and the constants of its type, drawing all the arguments again while
// none is a feature. An eligible primitive has an argument type the pool holds features of, so the draws end. There
// is none without eligible primitives, or when an argument's type has no candidate at all.
template<typename IGetNativePrimitive, typename IGetConstantsOfType, typename IRandomEngine>
struct derivation_sampler {
    derivation_sampler(
        IGetNativePrimitive& get_native_primitive,
        IGetConstantsOfType& get_constants_of_type,
        IRandomEngine& random_engine);
    std::optional<derivation> sample(
        const std::unordered_map<std::type_index, std::vector<feature_id>>& pool,
        const std::vector<prim_id>& eligible);
private:
    IGetNativePrimitive& get_native_primitive_;
    IGetConstantsOfType& get_constants_of_type_;
    IRandomEngine& random_engine_;
    std::vector<feature_id> none_;
};

#define DERIVATION_SAMPLER_TEMPLATE \
    template<typename IGetNativePrimitive, typename IGetConstantsOfType, typename IRandomEngine>
#define DERIVATION_SAMPLER derivation_sampler<IGetNativePrimitive, IGetConstantsOfType, IRandomEngine>

DERIVATION_SAMPLER_TEMPLATE
DERIVATION_SAMPLER::derivation_sampler(
    IGetNativePrimitive& get_native_primitive,
    IGetConstantsOfType& get_constants_of_type,
    IRandomEngine& random_engine)
    : get_native_primitive_(get_native_primitive)
    , get_constants_of_type_(get_constants_of_type)
    , random_engine_(random_engine)
    , none_() {}

DERIVATION_SAMPLER_TEMPLATE
std::optional<derivation> DERIVATION_SAMPLER::sample(
    const std::unordered_map<std::type_index, std::vector<feature_id>>& pool, const std::vector<prim_id>& eligible) {
    if (eligible.empty())
        return std::nullopt;
    std::uniform_int_distribution<size_t> primitives(0, eligible.size() - 1);
    const prim_id prim = eligible[primitives(random_engine_)];
    const std::vector<std::type_index>& arg_types = get_native_primitive_.primitive(prim).arg_types;
    std::vector<operand> args;
    bool fed = false;
    while (!fed) {
        args.clear();
        for (const std::type_index& type : arg_types) {
            const auto pooled = pool.find(type);
            const std::vector<feature_id>& features = pooled == pool.end() ? none_ : pooled->second;
            const std::vector<constant_id>& constants = get_constants_of_type_.of_type(type);
            const size_t candidate_count = features.size() + constants.size();
            if (candidate_count == 0)
                return std::nullopt;
            std::uniform_int_distribution<size_t> candidates(0, candidate_count - 1);
            const size_t candidate = candidates(random_engine_);
            const bool from_pool = candidate < features.size();
            fed = fed || from_pool;
            args.push_back(from_pool
                ? operand{operand::from_feature{.id = features[candidate]}}
                : operand{operand::from_constant{.id = constants[candidate - features.size()]}});
        }
    }
    return derivation{derivation::application{.prim = prim, .args = std::move(args)}};
}

#undef DERIVATION_SAMPLER
#undef DERIVATION_SAMPLER_TEMPLATE

}

#endif
