#ifndef MODEL_BUILDER_HPP
#define MODEL_BUILDER_HPP

#include <cstddef>
#include <optional>
#include <utility>
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
// each child bin is built in turn. The model is a tree the builder returns by value.
template<
    typename IDetectLeaf,
    typename ISynthesizeFeatures,
    typename IFindOutput,
    typename IChooseFeature,
    typename IChooseBoundArgument,
    typename ISplitBin>
struct model_builder {
    model_builder(
        IDetectLeaf& detect_leaf,
        ISynthesizeFeatures& synthesize_features,
        IFindOutput& find_output,
        IChooseFeature& choose_feature,
        IChooseBoundArgument& choose_bound_argument,
        ISplitBin& split_bin);
    model_node build(const bin& data);
private:
    model_node build_match(const bin& data, const candidate& scrutinee);

    IDetectLeaf& detect_leaf_;
    ISynthesizeFeatures& synthesize_features_;
    IFindOutput& find_output_;
    IChooseFeature& choose_feature_;
    IChooseBoundArgument& choose_bound_argument_;
    ISplitBin& split_bin_;
};

#define MODEL_BUILDER_TEMPLATE \
    template<typename IDL, typename ISF, typename IFO, typename ICFe, typename ICBA, typename ISB>
#define MODEL_BUILDER model_builder<IDL, ISF, IFO, ICFe, ICBA, ISB>

MODEL_BUILDER_TEMPLATE
MODEL_BUILDER::model_builder(
    IDL& detect_leaf,
    ISF& synthesize_features,
    IFO& find_output,
    ICFe& choose_feature,
    ICBA& choose_bound_argument,
    ISB& split_bin)
    : detect_leaf_(detect_leaf)
    , synthesize_features_(synthesize_features)
    , find_output_(find_output)
    , choose_feature_(choose_feature)
    , choose_bound_argument_(choose_bound_argument)
    , split_bin_(split_bin) {}

MODEL_BUILDER_TEMPLATE
model_node MODEL_BUILDER::build(const bin& data) {
    if (data.labels.cells.empty())
        return model_node{model_node::empty{}};
    std::optional<feature> detected = detect_leaf_.detect(data);
    if (detected.has_value())
        return model_node{model_node::leaf{.output = std::move(detected.value())}};

    const std::vector<candidate>& features = synthesize_features_.synthesize(data);
    std::optional<feature> synthesized = find_output_.find(features, data.labels);
    if (synthesized.has_value())
        return model_node{model_node::leaf{.output = std::move(synthesized.value())}};
    DEBUG_ASSERT(!features.empty());
    // A copy: building the children starts new sessions, which reuse the feature stores.
    const candidate scrutinee = features.at(choose_feature_.choose(features.size()));
    return build_match(data, scrutinee);
}

MODEL_BUILDER_TEMPLATE
model_node MODEL_BUILDER::build_match(const bin& data, const candidate& scrutinee) {
    const std::vector<bin> children = split_bin_.split(data, scrutinee.observed);
    size_t occupied_count = 0;
    for (const bin& child : children) {
        if (child.labels.cells.empty())
            continue;
        ++occupied_count;
    }
    DEBUG_ASSERT(occupied_count > 0);
    const bool funnels = occupied_count == 1;

    const var_id first_bound_argument = static_cast<var_id>(data.inputs.size());
    std::vector<model_node> branches;
    for (const bin& child : children) {
        if (!funnels || child.labels.cells.empty()) {
            branches.push_back(build(child));
            continue;
        }
        std::optional<feature> detected = detect_leaf_.detect(child);
        if (detected.has_value()) {
            branches.push_back(model_node{model_node::leaf{.output = std::move(detected.value())}});
            continue;
        }
        branches.push_back(build_match(child, choose_bound_argument_.choose(child, first_bound_argument)));
    }
    return model_node{model_node::match{.scrutinee = scrutinee.expression, .branches = std::move(branches)}};
}

#undef MODEL_BUILDER
#undef MODEL_BUILDER_TEMPLATE

#endif
