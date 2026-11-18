#ifndef FEATURE_SYNTHESIZER_HPP
#define FEATURE_SYNTHESIZER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/typed_column.hpp"

// Synthesizes the heterogeneous features of a bin bottom-up. Layer zero is the heterogeneous variables. Each later
// layer applies every primitive to argument tuples drawn from the constants and the features found so far, using at
// least one feature found by the previous layer, so every argument tuple holds a heterogeneous feature. Results that
// are homogeneous over the bin are dropped; the index keeps the cheapest feature per column.
template<
    typename IClearCandidates,
    typename ITryInsertCandidate,
    typename IGetCandidates,
    typename IFindVariableCandidates,
    typename IGetConstants,
    typename IGetPrimitiveCount,
    typename IGetPrimitiveSignature,
    typename IEnumerateArgumentTuples,
    typename IApplyPrimitiveToColumns,
    typename IFindUniformValue,
    typename IMakeAppFeature,
    typename ISizeFeature>
struct feature_synthesizer {
    feature_synthesizer(
        IClearCandidates& clear_candidates,
        ITryInsertCandidate& try_insert_candidate,
        IGetCandidates& get_candidates,
        IFindVariableCandidates& find_variable_candidates,
        IGetConstants& get_constants,
        IGetPrimitiveCount& get_primitive_count,
        IGetPrimitiveSignature& get_primitive_signature,
        IEnumerateArgumentTuples& enumerate_argument_tuples,
        IApplyPrimitiveToColumns& apply_primitive_to_columns,
        IFindUniformValue& find_uniform_value,
        IMakeAppFeature& make_app_feature,
        ISizeFeature& size_feature,
        size_t max_depth);
    std::vector<candidate> synthesize(const bin& data);
private:
    IClearCandidates& clear_candidates_;
    ITryInsertCandidate& try_insert_candidate_;
    IGetCandidates& get_candidates_;
    IFindVariableCandidates& find_variable_candidates_;
    IGetConstants& get_constants_;
    IGetPrimitiveCount& get_primitive_count_;
    IGetPrimitiveSignature& get_primitive_signature_;
    IEnumerateArgumentTuples& enumerate_argument_tuples_;
    IApplyPrimitiveToColumns& apply_primitive_to_columns_;
    IFindUniformValue& find_uniform_value_;
    IMakeAppFeature& make_app_feature_;
    ISizeFeature& size_feature_;
    size_t max_depth_;
};

#define FEATURE_SYNTHESIZER_TEMPLATE                                                                              \
    template<typename ICC, typename ITIC, typename IGC, typename IFVC, typename IGCo, typename IGPC, typename IGPS, \
             typename IEAT, typename IAPC, typename IFUV, typename IMAF, typename ISF>
#define FEATURE_SYNTHESIZER \
    feature_synthesizer<ICC, ITIC, IGC, IFVC, IGCo, IGPC, IGPS, IEAT, IAPC, IFUV, IMAF, ISF>

FEATURE_SYNTHESIZER_TEMPLATE
FEATURE_SYNTHESIZER::feature_synthesizer(
    ICC& clear_candidates,
    ITIC& try_insert_candidate,
    IGC& get_candidates,
    IFVC& find_variable_candidates,
    IGCo& get_constants,
    IGPC& get_primitive_count,
    IGPS& get_primitive_signature,
    IEAT& enumerate_argument_tuples,
    IAPC& apply_primitive_to_columns,
    IFUV& find_uniform_value,
    IMAF& make_app_feature,
    ISF& size_feature,
    size_t max_depth)
    : clear_candidates_(clear_candidates)
    , try_insert_candidate_(try_insert_candidate)
    , get_candidates_(get_candidates)
    , find_variable_candidates_(find_variable_candidates)
    , get_constants_(get_constants)
    , get_primitive_count_(get_primitive_count)
    , get_primitive_signature_(get_primitive_signature)
    , enumerate_argument_tuples_(enumerate_argument_tuples)
    , apply_primitive_to_columns_(apply_primitive_to_columns)
    , find_uniform_value_(find_uniform_value)
    , make_app_feature_(make_app_feature)
    , size_feature_(size_feature)
    , max_depth_(max_depth) {}

FEATURE_SYNTHESIZER_TEMPLATE
std::vector<candidate> FEATURE_SYNTHESIZER::synthesize(const bin& data) {
    clear_candidates_.clear();
    for (const candidate& variable : find_variable_candidates_.find(data, 0))
        try_insert_candidate_.try_insert(variable);

    std::vector<candidate> constants;
    for (const constant_operand& constant : get_constants_.constants()) {
        constants.push_back(candidate{
            .expression = constant.expression,
            .cost = size_feature_.size(constant.expression),
            .observed = typed_column{.type = constant.type, .cells = column(data.labels.size(), constant.literal)}});
    }

    size_t frontier_begin = 0;
    for (size_t depth = 0; depth < max_depth_; ++depth) {
        const std::vector<candidate> found = get_candidates_.candidates();
        const size_t frontier_end = found.size();
        if (frontier_begin == frontier_end)
            break;
        std::vector<candidate> operands = constants;
        operands.insert(operands.end(), found.begin(), found.end());

        for (prim_id prim = 0; prim < get_primitive_count_.count(); ++prim) {
            const primitive_signature signature = get_primitive_signature_.signature(prim);
            const std::vector<std::vector<size_t>> tuples = enumerate_argument_tuples_.enumerate(
                signature.arg_types, operands, constants.size() + frontier_begin, constants.size() + frontier_end);
            for (const std::vector<size_t>& tuple : tuples) {
                std::vector<const column*> arg_columns;
                std::vector<const feature*> arg_features;
                for (const size_t operand : tuple) {
                    arg_columns.push_back(&operands[operand].observed.cells);
                    arg_features.push_back(operands[operand].expression);
                }
                const column cells = apply_primitive_to_columns_.apply(prim, arg_columns);
                if (find_uniform_value_.find(cells).has_value())
                    continue;
                const feature* expression = make_app_feature_.make_app(prim, arg_features);
                try_insert_candidate_.try_insert(candidate{
                    .expression = expression,
                    .cost = size_feature_.size(expression),
                    .observed = typed_column{.type = signature.result_type, .cells = cells}});
            }
        }
        frontier_begin = frontier_end;
    }
    return get_candidates_.candidates();
}

#undef FEATURE_SYNTHESIZER
#undef FEATURE_SYNTHESIZER_TEMPLATE

#endif
