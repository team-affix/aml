#ifndef INHERITING_BIN_SPLITTER_HPP
#define INHERITING_BIN_SPLITTER_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// Splits a bin as the bin splitter it wraps does, and records what each child inherits: what its parent inherited,
// then the scrutinee's expression and every primitive application inside it, each once. The scrutinee's expression is
// the session's candidate that observed the scrutinee column; a column no candidate observed (a funnel's bound
// argument, a variable already in scope) passes nothing new. A parent without a record is the root of a new model, so
// the records of the last model are cleared first.
template<
    typename ISplitBin,
    typename IGetFeatureCandidates,
    typename ICheckInherited,
    typename IGetInheritedFeatures,
    typename IClearInheritedFeatures,
    typename IRecordInheritedFeatures>
struct inheriting_bin_splitter {
    inheriting_bin_splitter(
        ISplitBin& split_bin,
        IGetFeatureCandidates& get_feature_candidates,
        ICheckInherited& check_inherited,
        IGetInheritedFeatures& get_inherited_features,
        IClearInheritedFeatures& clear_inherited_features,
        IRecordInheritedFeatures& record_inherited_features);
    std::vector<bin_view> split(const bin_view& parent, const column_view& scrutinee);
private:
    void collect_applications(const feature& expression, std::vector<feature>& collected) const;

    ISplitBin& split_bin_;
    IGetFeatureCandidates& get_feature_candidates_;
    ICheckInherited& check_inherited_;
    IGetInheritedFeatures& get_inherited_features_;
    IClearInheritedFeatures& clear_inherited_features_;
    IRecordInheritedFeatures& record_inherited_features_;
};

#define INHERITING_BIN_SPLITTER_TEMPLATE \
    template<typename ISB, typename IGFC, typename ICI, typename IGIF, typename ICIF, typename IRIF>
#define INHERITING_BIN_SPLITTER inheriting_bin_splitter<ISB, IGFC, ICI, IGIF, ICIF, IRIF>

INHERITING_BIN_SPLITTER_TEMPLATE
INHERITING_BIN_SPLITTER::inheriting_bin_splitter(
    ISB& split_bin,
    IGFC& get_feature_candidates,
    ICI& check_inherited,
    IGIF& get_inherited_features,
    ICIF& clear_inherited_features,
    IRIF& record_inherited_features)
    : split_bin_(split_bin)
    , get_feature_candidates_(get_feature_candidates)
    , check_inherited_(check_inherited)
    , get_inherited_features_(get_inherited_features)
    , clear_inherited_features_(clear_inherited_features)
    , record_inherited_features_(record_inherited_features) {}

INHERITING_BIN_SPLITTER_TEMPLATE
std::vector<bin_view> INHERITING_BIN_SPLITTER::split(const bin_view& parent, const column_view& scrutinee) {
    const std::vector<size_t>* parent_rows = parent.labels.rows.get();
    const bool root = !check_inherited_.has(parent_rows);
    if (root)
        clear_inherited_features_.clear();
    std::vector<feature> passed = root ? std::vector<feature>{} : get_inherited_features_.inherited(parent_rows);
    for (const candidate& held : get_feature_candidates_.candidates()) {
        const bool observed_it = held.observed.cells == scrutinee.cells && held.observed.rows == scrutinee.rows;
        if (!observed_it)
            continue;
        collect_applications(held.expression, passed);
        break;
    }
    std::vector<bin_view> children = split_bin_.split(parent, scrutinee);
    for (const bin_view& child : children)
        record_inherited_features_.record(child.labels.rows.get(), passed);
    return children;
}

INHERITING_BIN_SPLITTER_TEMPLATE
void INHERITING_BIN_SPLITTER::collect_applications(const feature& expression, std::vector<feature>& collected) const {
    const feature::app* application = std::get_if<feature::app>(&expression.content);
    if (application == nullptr)
        return;
    bool held = false;
    for (const feature& earlier : collected)
        held = held || earlier == expression;
    if (!held)
        collected.push_back(expression);
    for (const feature& argument : application->args)
        collect_applications(argument, collected);
}

#undef INHERITING_BIN_SPLITTER
#undef INHERITING_BIN_SPLITTER_TEMPLATE

}

#endif
