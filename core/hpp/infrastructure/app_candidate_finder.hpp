#ifndef APP_CANDIDATE_FINDER_HPP
#define APP_CANDIDATE_FINDER_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/typed_column.hpp"

// Finds the application of a primitive to a tuple of operand indices (the constants first, then the session's
// features) as a candidate. An application that is homogeneous over the rows gives none.
template<
    typename IApplyPrimitiveToOperands,
    typename IFindUniformValue,
    typename IGetConstants,
    typename IGetFeatureCandidates,
    typename IMakeAppFeature,
    typename ISizeFeature>
struct app_candidate_finder {
    app_candidate_finder(
        IApplyPrimitiveToOperands& apply_primitive_to_operands,
        IFindUniformValue& find_uniform_value,
        IGetConstants& get_constants,
        IGetFeatureCandidates& get_feature_candidates,
        IMakeAppFeature& make_app_feature,
        ISizeFeature& size_feature);
    std::optional<candidate> find(
        prim_id prim, type_id result_type, const std::vector<size_t>& args, size_t row_count);
private:
    IApplyPrimitiveToOperands& apply_primitive_to_operands_;
    IFindUniformValue& find_uniform_value_;
    IGetConstants& get_constants_;
    IGetFeatureCandidates& get_feature_candidates_;
    IMakeAppFeature& make_app_feature_;
    ISizeFeature& size_feature_;
};

#define APP_CANDIDATE_FINDER_TEMPLATE \
    template<typename IAPO, typename IFUV, typename IGC, typename IGFC, typename IMAF, typename ISF>
#define APP_CANDIDATE_FINDER app_candidate_finder<IAPO, IFUV, IGC, IGFC, IMAF, ISF>

APP_CANDIDATE_FINDER_TEMPLATE
APP_CANDIDATE_FINDER::app_candidate_finder(
    IAPO& apply_primitive_to_operands,
    IFUV& find_uniform_value,
    IGC& get_constants,
    IGFC& get_feature_candidates,
    IMAF& make_app_feature,
    ISF& size_feature)
    : apply_primitive_to_operands_(apply_primitive_to_operands)
    , find_uniform_value_(find_uniform_value)
    , get_constants_(get_constants)
    , get_feature_candidates_(get_feature_candidates)
    , make_app_feature_(make_app_feature)
    , size_feature_(size_feature) {}

APP_CANDIDATE_FINDER_TEMPLATE
std::optional<candidate> APP_CANDIDATE_FINDER::find(
    prim_id prim, type_id result_type, const std::vector<size_t>& args, size_t row_count) {
    column cells = apply_primitive_to_operands_.apply(prim, args, row_count);
    if (find_uniform_value_.find(cells).has_value())
        return std::nullopt;

    const std::vector<constant_operand>& constants = get_constants_.constants();
    const std::vector<candidate>& features = get_feature_candidates_.candidates();
    std::vector<const feature*> arg_features;
    for (const size_t operand : args) {
        const bool is_constant = operand < constants.size();
        arg_features.push_back(
            is_constant ? constants[operand].expression : features[operand - constants.size()].expression);
    }
    const feature* expression = make_app_feature_.make_app(prim, arg_features);
    return candidate{
        .expression = expression,
        .cost = size_feature_.size(expression),
        .observed = typed_column{.type = result_type, .cells = std::move(cells)}};
}

#undef APP_CANDIDATE_FINDER
#undef APP_CANDIDATE_FINDER_TEMPLATE

#endif
