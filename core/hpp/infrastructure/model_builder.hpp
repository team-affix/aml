#ifndef MODEL_BUILDER_HPP
#define MODEL_BUILDER_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/var_id.hpp"
#include "debug_assert.hpp"

// Builds a model that fits a bin exactly. A bin without rows becomes empty. A bin with a detected output (a variable
// equal to its labels, or labels that agree) becomes a leaf. Otherwise a synthesis session runs over the bin: a
// synthesized feature equal to the labels becomes a leaf's output, and failing that the bin is partitioned by a match
// on a chosen feature. While every row falls into one constructor the match funnels into a chosen bound argument that
// constructor just bound, unless a variable already gives the output; once rows fall into two or more constructors,
// each child bin is built in turn.
template<
    typename IDetectLeaf,
    typename ISynthesizeFeatures,
    typename IFindOutput,
    typename IChooseFeature,
    typename IChooseBoundArgument,
    typename ISplitBin,
    typename IMakeEmpty,
    typename IMakeLeaf,
    typename IMakeMatch>
struct model_builder {
    model_builder(
        IDetectLeaf& detect_leaf,
        ISynthesizeFeatures& synthesize_features,
        IFindOutput& find_output,
        IChooseFeature& choose_feature,
        IChooseBoundArgument& choose_bound_argument,
        ISplitBin& split_bin,
        IMakeEmpty& make_empty,
        IMakeLeaf& make_leaf,
        IMakeMatch& make_match);
    const model_node* build(const bin& data);
private:
    const model_node* build_match(const bin& data, const candidate& scrutinee);

    IDetectLeaf& detect_leaf_;
    ISynthesizeFeatures& synthesize_features_;
    IFindOutput& find_output_;
    IChooseFeature& choose_feature_;
    IChooseBoundArgument& choose_bound_argument_;
    ISplitBin& split_bin_;
    IMakeEmpty& make_empty_;
    IMakeLeaf& make_leaf_;
    IMakeMatch& make_match_;
};

#define MODEL_BUILDER_TEMPLATE                                                                                   \
    template<typename IDL, typename ISF, typename IFO, typename ICFe, typename ICBA, typename ISB, typename IME, \
             typename IML, typename IMM>
#define MODEL_BUILDER model_builder<IDL, ISF, IFO, ICFe, ICBA, ISB, IME, IML, IMM>

MODEL_BUILDER_TEMPLATE
MODEL_BUILDER::model_builder(
    IDL& detect_leaf,
    ISF& synthesize_features,
    IFO& find_output,
    ICFe& choose_feature,
    ICBA& choose_bound_argument,
    ISB& split_bin,
    IME& make_empty,
    IML& make_leaf,
    IMM& make_match)
    : detect_leaf_(detect_leaf)
    , synthesize_features_(synthesize_features)
    , find_output_(find_output)
    , choose_feature_(choose_feature)
    , choose_bound_argument_(choose_bound_argument)
    , split_bin_(split_bin)
    , make_empty_(make_empty)
    , make_leaf_(make_leaf)
    , make_match_(make_match) {}

MODEL_BUILDER_TEMPLATE
const model_node* MODEL_BUILDER::build(const bin& data) {
    if (data.labels.empty())
        return make_empty_.make_empty();
    const std::optional<const feature*> detected = detect_leaf_.detect(data);
    if (detected.has_value())
        return make_leaf_.make_leaf(detected.value());

    const std::vector<candidate>& features = synthesize_features_.synthesize(data);
    const std::optional<const feature*> synthesized = find_output_.find(features, data.labels);
    if (synthesized.has_value())
        return make_leaf_.make_leaf(synthesized.value());
    DEBUG_ASSERT(!features.empty());
    // A copy: building the children starts new sessions, which reuse the feature stores.
    const candidate scrutinee = features.at(choose_feature_.choose(features.size()));
    return build_match(data, scrutinee);
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

    const var_id first_bound_argument = static_cast<var_id>(data.inputs.size());
    std::vector<const model_node*> branches;
    for (const bin& child : children) {
        if (!funnels || child.labels.empty()) {
            branches.push_back(build(child));
            continue;
        }
        const std::optional<const feature*> detected = detect_leaf_.detect(child);
        if (detected.has_value()) {
            branches.push_back(make_leaf_.make_leaf(detected.value()));
            continue;
        }
        branches.push_back(build_match(child, choose_bound_argument_.choose(child, first_bound_argument)));
    }
    return make_match_.make_match(scrutinee.expression, branches);
}

#undef MODEL_BUILDER
#undef MODEL_BUILDER_TEMPLATE

#endif
