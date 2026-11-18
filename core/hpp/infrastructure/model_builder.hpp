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
// leaf. Any other bin is partitioned by a match on a chosen scrutinee. While every row falls into one constructor the
// match funnels into a chosen field that constructor just bound; once rows fall into two or more constructors, each
// child bin is built in turn.
template<
    typename IDetectLeaf,
    typename IChooseScrutinee,
    typename IChooseField,
    typename ISplitBin,
    typename IMakeEmpty,
    typename IMakeLeaf,
    typename IMakeMatch>
struct model_builder {
    model_builder(
        IDetectLeaf& detect_leaf,
        IChooseScrutinee& choose_scrutinee,
        IChooseField& choose_field,
        ISplitBin& split_bin,
        IMakeEmpty& make_empty,
        IMakeLeaf& make_leaf,
        IMakeMatch& make_match);
    const model_node* build(const bin& data);
private:
    const model_node* build_match(const bin& data, const candidate& scrutinee);

    IDetectLeaf& detect_leaf_;
    IChooseScrutinee& choose_scrutinee_;
    IChooseField& choose_field_;
    ISplitBin& split_bin_;
    IMakeEmpty& make_empty_;
    IMakeLeaf& make_leaf_;
    IMakeMatch& make_match_;
};

#define MODEL_BUILDER_TEMPLATE \
    template<typename IDL, typename ICS, typename ICF, typename ISB, typename IME, typename IML, typename IMM>
#define MODEL_BUILDER model_builder<IDL, ICS, ICF, ISB, IME, IML, IMM>

MODEL_BUILDER_TEMPLATE
MODEL_BUILDER::model_builder(
    IDL& detect_leaf,
    ICS& choose_scrutinee,
    ICF& choose_field,
    ISB& split_bin,
    IME& make_empty,
    IML& make_leaf,
    IMM& make_match)
    : detect_leaf_(detect_leaf)
    , choose_scrutinee_(choose_scrutinee)
    , choose_field_(choose_field)
    , split_bin_(split_bin)
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
    return build_match(data, choose_scrutinee_.choose(data));
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

    const var_id first_field = static_cast<var_id>(data.scope.size());
    std::vector<const model_node*> branches;
    for (const bin& child : children) {
        if (!funnels || child.labels.empty()) {
            branches.push_back(build(child));
            continue;
        }
        branches.push_back(build_match(child, choose_field_.choose(child, first_field)));
    }
    return make_match_.make_match(scrutinee.expression, branches);
}

#undef MODEL_BUILDER
#undef MODEL_BUILDER_TEMPLATE

#endif
