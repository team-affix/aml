#ifndef MODEL_BUILDER_HPP
#define MODEL_BUILDER_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/value.hpp"
#include "value_objects/var_id.hpp"
#include "debug_assert.hpp"

// Builds a model that fits a bin exactly. A bin without rows becomes empty and a bin whose labels agree becomes a
// leaf. Any other bin is partitioned: a chosen heterogeneous feature is matched on, and while every row falls into
// one constructor the match funnels into a chosen heterogeneous field that constructor just bound. Once rows fall
// into two or more constructors, each child bin is built in turn.
template<
    typename IDetectLeaf,
    typename ISynthesizeCandidates,
    typename IChooseFeature,
    typename ISplitBin,
    typename IFindVariableCandidates,
    typename IChooseField,
    typename IMakeEmpty,
    typename IMakeLeaf,
    typename IMakeMatch>
struct model_builder {
    model_builder(
        IDetectLeaf& detect_leaf,
        ISynthesizeCandidates& synthesize_candidates,
        IChooseFeature& choose_feature,
        ISplitBin& split_bin,
        IFindVariableCandidates& find_variable_candidates,
        IChooseField& choose_field,
        IMakeEmpty& make_empty,
        IMakeLeaf& make_leaf,
        IMakeMatch& make_match);
    const model_node* build(const bin& data);
private:
    const model_node* build_match(const bin& data, const candidate& scrutinee);

    IDetectLeaf& detect_leaf_;
    ISynthesizeCandidates& synthesize_candidates_;
    IChooseFeature& choose_feature_;
    ISplitBin& split_bin_;
    IFindVariableCandidates& find_variable_candidates_;
    IChooseField& choose_field_;
    IMakeEmpty& make_empty_;
    IMakeLeaf& make_leaf_;
    IMakeMatch& make_match_;
};

#define MODEL_BUILDER_TEMPLATE                                                                             \
    template<typename IDL, typename ISC, typename ICFe, typename ISB, typename IFVC, typename ICFi, \
             typename IME, typename IML, typename IMM>
#define MODEL_BUILDER model_builder<IDL, ISC, ICFe, ISB, IFVC, ICFi, IME, IML, IMM>

MODEL_BUILDER_TEMPLATE
MODEL_BUILDER::model_builder(
    IDL& detect_leaf,
    ISC& synthesize_candidates,
    ICFe& choose_feature,
    ISB& split_bin,
    IFVC& find_variable_candidates,
    ICFi& choose_field,
    IME& make_empty,
    IML& make_leaf,
    IMM& make_match)
    : detect_leaf_(detect_leaf)
    , synthesize_candidates_(synthesize_candidates)
    , choose_feature_(choose_feature)
    , split_bin_(split_bin)
    , find_variable_candidates_(find_variable_candidates)
    , choose_field_(choose_field)
    , make_empty_(make_empty)
    , make_leaf_(make_leaf)
    , make_match_(make_match) {}

MODEL_BUILDER_TEMPLATE
const model_node* MODEL_BUILDER::build(const bin& data) {
    if (data.labels.empty())
        return make_empty_.make_empty();
    const std::optional<const value*> label = detect_leaf_.detect(data);
    if (label.has_value())
        return make_leaf_.make_leaf(label.value());
    const std::vector<candidate> candidates = synthesize_candidates_.synthesize(data);
    DEBUG_ASSERT(!candidates.empty());
    return build_match(data, candidates.at(choose_feature_.choose(candidates)));
}

MODEL_BUILDER_TEMPLATE
const model_node* MODEL_BUILDER::build_match(const bin& data, const candidate& scrutinee) {
    const std::vector<bin> children = split_bin_.split(data, scrutinee.observed);
    size_t occupied_count = 0;
    for (const bin& child : children) {
        if (child.labels.empty())
            continue;
        ++occupied_count;
    }
    DEBUG_ASSERT(occupied_count > 0);
    const bool funnels = occupied_count == 1;

    std::vector<const model_node*> branches;
    for (const bin& child : children) {
        if (!funnels || child.labels.empty()) {
            branches.push_back(build(child));
            continue;
        }
        const var_id first_field = static_cast<var_id>(data.scope.size());
        const std::vector<candidate> fields = find_variable_candidates_.find(child, first_field);
        DEBUG_ASSERT(!fields.empty());
        branches.push_back(build_match(child, fields.at(choose_field_.choose(fields))));
    }
    return make_match_.make_match(scrutinee.expression, branches);
}

#undef MODEL_BUILDER
#undef MODEL_BUILDER_TEMPLATE

#endif
