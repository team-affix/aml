#ifndef APP_CANDIDATE_FINDER_HPP
#define APP_CANDIDATE_FINDER_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/typed_column.hpp"

// Finds the application of a primitive to a tuple of operands as a candidate. An application that is homogeneous over
// the rows gives none.
template<
    typename IApplyPrimitiveToOperands,
    typename IFindUniformValue,
    typename IGetOperandExpression,
    typename ISizeFeature>
struct app_candidate_finder {
    app_candidate_finder(
        IApplyPrimitiveToOperands& apply_primitive_to_operands,
        IFindUniformValue& find_uniform_value,
        IGetOperandExpression& get_operand_expression,
        ISizeFeature& size_feature);
    std::optional<candidate> find(
        prim_id prim, type_id result_type, const std::vector<size_t>& args, size_t row_count);
private:
    IApplyPrimitiveToOperands& apply_primitive_to_operands_;
    IFindUniformValue& find_uniform_value_;
    IGetOperandExpression& get_operand_expression_;
    ISizeFeature& size_feature_;
};

#define APP_CANDIDATE_FINDER_TEMPLATE template<typename IAPO, typename IFUV, typename IGOE, typename ISF>
#define APP_CANDIDATE_FINDER          app_candidate_finder<IAPO, IFUV, IGOE, ISF>

APP_CANDIDATE_FINDER_TEMPLATE
APP_CANDIDATE_FINDER::app_candidate_finder(
    IAPO& apply_primitive_to_operands, IFUV& find_uniform_value, IGOE& get_operand_expression, ISF& size_feature)
    : apply_primitive_to_operands_(apply_primitive_to_operands)
    , find_uniform_value_(find_uniform_value)
    , get_operand_expression_(get_operand_expression)
    , size_feature_(size_feature) {}

APP_CANDIDATE_FINDER_TEMPLATE
std::optional<candidate> APP_CANDIDATE_FINDER::find(
    prim_id prim, type_id result_type, const std::vector<size_t>& args, size_t row_count) {
    column cells = apply_primitive_to_operands_.apply(prim, args, row_count);
    if (find_uniform_value_.find(cells).has_value())
        return std::nullopt;

    std::vector<feature> arg_features;
    for (const size_t operand : args)
        arg_features.push_back(get_operand_expression_.expression(operand));
    feature expression{feature::app{.prim = prim, .args = std::move(arg_features)}};
    const size_t cost = size_feature_.size(expression);
    return candidate{
        .expression = std::move(expression),
        .cost = cost,
        .observed = typed_column{.type = result_type, .cells = std::move(cells)}};
}

#undef APP_CANDIDATE_FINDER
#undef APP_CANDIDATE_FINDER_TEMPLATE

#endif
