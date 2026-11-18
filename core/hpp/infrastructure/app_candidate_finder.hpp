#ifndef APP_CANDIDATE_FINDER_HPP
#define APP_CANDIDATE_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/typed_column.hpp"

// Finds the application of a primitive to argument candidates as a candidate, applying it row by row to their
// columns. An application that is homogeneous over the rows gives none.
template<
    typename IApplyPrimitiveToColumns, typename IFindUniformValue, typename IMakeAppFeature, typename ISizeFeature>
struct app_candidate_finder {
    app_candidate_finder(
        IApplyPrimitiveToColumns& apply_primitive_to_columns,
        IFindUniformValue& find_uniform_value,
        IMakeAppFeature& make_app_feature,
        ISizeFeature& size_feature);
    std::optional<candidate> find(prim_id prim, type_id result_type, const std::vector<const candidate*>& args);
private:
    IApplyPrimitiveToColumns& apply_primitive_to_columns_;
    IFindUniformValue& find_uniform_value_;
    IMakeAppFeature& make_app_feature_;
    ISizeFeature& size_feature_;
};

#define APP_CANDIDATE_FINDER_TEMPLATE template<typename IAPC, typename IFUV, typename IMAF, typename ISF>
#define APP_CANDIDATE_FINDER          app_candidate_finder<IAPC, IFUV, IMAF, ISF>

APP_CANDIDATE_FINDER_TEMPLATE
APP_CANDIDATE_FINDER::app_candidate_finder(
    IAPC& apply_primitive_to_columns, IFUV& find_uniform_value, IMAF& make_app_feature, ISF& size_feature)
    : apply_primitive_to_columns_(apply_primitive_to_columns)
    , find_uniform_value_(find_uniform_value)
    , make_app_feature_(make_app_feature)
    , size_feature_(size_feature) {}

APP_CANDIDATE_FINDER_TEMPLATE
std::optional<candidate> APP_CANDIDATE_FINDER::find(
    prim_id prim, type_id result_type, const std::vector<const candidate*>& args) {
    std::vector<const column*> arg_columns;
    for (const candidate* arg : args)
        arg_columns.push_back(&arg->observed.cells);
    const column cells = apply_primitive_to_columns_.apply(prim, arg_columns);
    if (find_uniform_value_.find(cells).has_value())
        return std::nullopt;

    std::vector<const feature*> arg_features;
    for (const candidate* arg : args)
        arg_features.push_back(arg->expression);
    const feature* expression = make_app_feature_.make_app(prim, arg_features);
    return candidate{
        .expression = expression,
        .cost = size_feature_.size(expression),
        .observed = typed_column{.type = result_type, .cells = cells}};
}

#undef APP_CANDIDATE_FINDER
#undef APP_CANDIDATE_FINDER_TEMPLATE

#endif
