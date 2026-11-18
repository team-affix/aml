#ifndef FEATURE_SYNTHESIZER_HPP
#define FEATURE_SYNTHESIZER_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"

// Synthesizes the heterogeneous features of a bin bottom-up. Layer zero is the heterogeneous variables. Each later
// layer applies every primitive to argument tuples drawn from the constants and the features found so far, using at
// least one feature found by the previous layer, so every argument tuple holds a heterogeneous feature. Applications
// that are homogeneous over the bin are dropped; the index keeps the cheapest feature per column.
template<
    typename IClearCandidates,
    typename ITryInsertCandidate,
    typename IGetCandidates,
    typename IFindVariableCandidates,
    typename IFindConstantCandidates,
    typename IGetPrimitiveCount,
    typename IGetPrimitiveSignature,
    typename IEnumerateArgumentTuples,
    typename IFindAppCandidate>
struct feature_synthesizer {
    feature_synthesizer(
        IClearCandidates& clear_candidates,
        ITryInsertCandidate& try_insert_candidate,
        IGetCandidates& get_candidates,
        IFindVariableCandidates& find_variable_candidates,
        IFindConstantCandidates& find_constant_candidates,
        IGetPrimitiveCount& get_primitive_count,
        IGetPrimitiveSignature& get_primitive_signature,
        IEnumerateArgumentTuples& enumerate_argument_tuples,
        IFindAppCandidate& find_app_candidate,
        size_t max_depth);
    std::vector<candidate> synthesize(const bin& data);
private:
    IClearCandidates& clear_candidates_;
    ITryInsertCandidate& try_insert_candidate_;
    IGetCandidates& get_candidates_;
    IFindVariableCandidates& find_variable_candidates_;
    IFindConstantCandidates& find_constant_candidates_;
    IGetPrimitiveCount& get_primitive_count_;
    IGetPrimitiveSignature& get_primitive_signature_;
    IEnumerateArgumentTuples& enumerate_argument_tuples_;
    IFindAppCandidate& find_app_candidate_;
    size_t max_depth_;
};

#define FEATURE_SYNTHESIZER_TEMPLATE                                                                              \
    template<typename ICC, typename ITIC, typename IGC, typename IFVC, typename IFCC, typename IGPC, typename IGPS, \
             typename IEAT, typename IFAC>
#define FEATURE_SYNTHESIZER feature_synthesizer<ICC, ITIC, IGC, IFVC, IFCC, IGPC, IGPS, IEAT, IFAC>

FEATURE_SYNTHESIZER_TEMPLATE
FEATURE_SYNTHESIZER::feature_synthesizer(
    ICC& clear_candidates,
    ITIC& try_insert_candidate,
    IGC& get_candidates,
    IFVC& find_variable_candidates,
    IFCC& find_constant_candidates,
    IGPC& get_primitive_count,
    IGPS& get_primitive_signature,
    IEAT& enumerate_argument_tuples,
    IFAC& find_app_candidate,
    size_t max_depth)
    : clear_candidates_(clear_candidates)
    , try_insert_candidate_(try_insert_candidate)
    , get_candidates_(get_candidates)
    , find_variable_candidates_(find_variable_candidates)
    , find_constant_candidates_(find_constant_candidates)
    , get_primitive_count_(get_primitive_count)
    , get_primitive_signature_(get_primitive_signature)
    , enumerate_argument_tuples_(enumerate_argument_tuples)
    , find_app_candidate_(find_app_candidate)
    , max_depth_(max_depth) {}

FEATURE_SYNTHESIZER_TEMPLATE
std::vector<candidate> FEATURE_SYNTHESIZER::synthesize(const bin& data) {
    clear_candidates_.clear();
    for (const candidate& variable : find_variable_candidates_.find(data, 0))
        try_insert_candidate_.try_insert(variable);
    const std::vector<candidate> constants = find_constant_candidates_.find(data);

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
                std::vector<const candidate*> args;
                for (const size_t operand : tuple)
                    args.push_back(&operands[operand]);
                const std::optional<candidate> application =
                    find_app_candidate_.find(prim, signature.result_type, args);
                if (!application.has_value())
                    continue;
                try_insert_candidate_.try_insert(application.value());
            }
        }
        frontier_begin = frontier_end;
    }
    return get_candidates_.candidates();
}

#undef FEATURE_SYNTHESIZER
#undef FEATURE_SYNTHESIZER_TEMPLATE

#endif
